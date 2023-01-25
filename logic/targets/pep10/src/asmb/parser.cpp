#include "parser.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/range/adaptor/indexed.hpp>
#include <set>
#include <utility>

#include "../isa/pep10.hpp"
#include "./directives.hpp"
#include "./ir.hpp"
#include "asmdr/frontend/preprocesser.hpp"
#include "asmdr/frontend/tokenizer.hpp"
#include "ir/args.hpp"
#include "ir/directives.hpp"
#include "ir/empty.hpp"
#include "ir/macro.hpp"
#include "symbol/entry.hpp"
#include "symbol/value.hpp"
//#include "masm/ir.macro.hpp"

bool asmb::pep10::requires_byte_operand(isa::pep10::instruction_mnemonic mn) {
    return mn == isa::pep10::instruction_mnemonic::LDBA || mn == isa::pep10::instruction_mnemonic::LDBX;
}
bool asmb::pep10::requires_addr_mode(isa::pep10::instruction_mnemonic mn) {
    return !(mn == isa::pep10::instruction_mnemonic::BR || mn == isa::pep10::instruction_mnemonic::BRLT ||
             mn == isa::pep10::instruction_mnemonic::BRLE || mn == isa::pep10::instruction_mnemonic::BRNE ||
             mn == isa::pep10::instruction_mnemonic::BREQ || mn == isa::pep10::instruction_mnemonic::BRGT ||
             mn == isa::pep10::instruction_mnemonic::BRGE || mn == isa::pep10::instruction_mnemonic::BRV ||
             mn == isa::pep10::instruction_mnemonic::BRC || mn == isa::pep10::instruction_mnemonic::CALL);
}
std::optional<isa::pep10::addressing_mode> asmb::pep10::parse_addr_mode(std::string addr_mode) {
    boost::algorithm::to_lower(addr_mode);
    if (addr_mode == "i")
        return isa::pep10::addressing_mode::I;
    else if (addr_mode == "d")
        return isa::pep10::addressing_mode::D;
    else if (addr_mode == "x")
        return isa::pep10::addressing_mode::X;
    else if (addr_mode == "n")
        return isa::pep10::addressing_mode::N;
    else if (addr_mode == "s")
        return isa::pep10::addressing_mode::S;
    else if (addr_mode == "sf")
        return isa::pep10::addressing_mode::SF;
    else if (addr_mode == "sx")
        return isa::pep10::addressing_mode::SX;
    else if (addr_mode == "sfx")
        return isa::pep10::addressing_mode::SFX;
    else
        return isa::pep10::addressing_mode::INVALID;
}

bool asmb::pep10::allowed_addressing_mode(isa::pep10::instruction_mnemonic mn, isa::pep10::addressing_mode am) {
    using namespace isa::pep10;
    auto isa = isa::pep10::isa_definition::get_definition();
    auto def_ptr = isa.isa.find(mn);
    if (def_ptr == isa.isa.end())
        return false;
    switch (def_ptr->second->iformat) {
    case isa::pep10::addressing_class::U_none:
    case isa::pep10::addressing_class::R_none:
        return false;
    case isa::pep10::addressing_class::A_ix:
        return am == addressing_mode::I || am == addressing_mode::X;
    case isa::pep10::addressing_class::RAAA_all:
    case isa::pep10::addressing_class::AAA_all:
        return true;
    case isa::pep10::addressing_class::AAA_i:
        return am == addressing_mode::I;
    case isa::pep10::addressing_class::RAAA_noi:
        return am != addressing_mode::I;
    default:
        throw std::logic_error("Invalid addressing mode.");
    }
}

bool asmb::pep10::valid_symbol_name(const std::string &symbol) {
    // TODO: Determine what makes a valid symbol;
    if (symbol.size() > 8)
        return false;
    return true;
}

auto asmb::pep10::parser::parse(std::shared_ptr<masm::project::target<uint16_t>> &target,
                                std::shared_ptr<ir::section::code_section<uint16_t>> &section) -> bool {
    using token_class_t = const std::set<masm::frontend::token_type>;
    static const token_class_t comment = {masm::frontend::token_type::kComment};
    static const token_class_t empty = {masm::frontend::token_type::kEmpty};
    static const token_class_t symbol = {masm::frontend::token_type::kSymbolDecl};
    static const token_class_t identifier = {masm::frontend::token_type::kIdentifier};
    static const token_class_t dot = {masm::frontend::token_type::kDotDirective};
    static const token_class_t macro = {masm::frontend::token_type::kMacroInvoke};
    static const token_class_t comma = {masm::frontend::token_type::kComma};
    // Must also update masm::frontend::preprocessor
    static const token_class_t macro_args = {
        masm::frontend::token_type::kIdentifier, masm::frontend::token_type::kDecConstant,
        masm::frontend::token_type::kHexConstant, masm::frontend::token_type::kCharConstant,
        masm::frontend::token_type::kStrConstant};

    bool success = true;
    int burn_count = 0, end_count = 0;
    auto symbol_table = section->symbol_table;
    decltype(section->ir_lines) ir_lines;
    auto tokens = target->section_intermediates.find(section);
    assert(tokens != target->section_intermediates.end());

    for (auto [index, line] : tokens->second.tokenized.value().tokens | boost::adaptors::indexed(0)) {
        auto start = line.begin(), last = line.end();
        bool local_success = true;
        std::string local_message = {};
        ir_pointer_t local_line = nullptr;
        std::shared_ptr<symbol::entry<uint16_t>> local_symbol = nullptr;

        // Check if we comment or empty line. If so, we can skip a lot of processing.
        if (auto [match_comment, _1, text_comment] = masm::frontend::match(start, last, comment, true); match_comment) {
            // Require that line end in empty token.
            auto [match_empty, _2, _3] =
                masm::frontend::match(start, last, empty, true); // NOLINT: Matches have nothing useful.
            local_line = std::make_shared<ir::comment_line<uint16_t>>();
            local_line->comment = text_comment;
            local_line->source_line = index;
            ir_lines.emplace_back(local_line);
            continue;
        } else if (auto [match_empty, _2, _3] = masm::frontend::match(start, last, empty, true); match_empty) {
            local_line = std::make_shared<ir::blank_line<uint16_t>>();
            local_line->source_line = index;
            ir_lines.emplace_back(local_line);
            continue;
        }

        // Extract symbol declaration if present,
        if (auto [match_symbol, _1, text_symbol] = masm::frontend::match(start, last, symbol, true); match_symbol) {
            local_symbol = symbol_table->define(text_symbol);
        }

        // Begin parsing for mnemonics, dot commands, and macros.
        if (auto [match_ident, _1, text_identifier] = masm::frontend::match(start, last, identifier, true);
            match_ident) {
            // Check if identifier is a mnemonic. Otherwise, error.
            auto isa_def = isa::pep10::isa_definition::get_definition();
            auto it = std::find_if(isa_def.isa.cbegin(), isa_def.isa.cend(), [ti = text_identifier](auto entry) {
                return boost::iequals(isa::pep10::as_string(entry.first), ti);
            });

            if (it == isa_def.isa.cend())
                std::tie(local_success, local_message) =
                    std::make_tuple(false, ";ERROR: Invalid instruction mnemonic.");
            else if (auto mnemonic = it->first; isa::pep10::is_mnemonic_unary(mnemonic)) {
                std::tie(local_success, local_message, local_line) = parse_unary(start, last, mnemonic);
            } else {
                std::tie(local_success, local_message, local_line) =
                    parse_nonunary(start, last, symbol_table, mnemonic);
            }

            success &= local_success;
            if (!local_success) {
                target->message_resolver->log_message(section, index, {masm::message_type::kError, local_message});
                continue;
            } else if (local_symbol)
                local_line->symbol_entry = local_symbol;

        }
        // TODO: Fix case sensitivity on dot commands.
        else if (auto [match_dot, _2, text_dot] = masm::frontend::match(start, last, dot, true); match_dot) {
            if (boost::iequals(text_dot, "ASCII"))
                std::tie(local_success, local_message, local_line) = parse_ASCII(start, last);
            else if (boost::iequals(text_dot, "ALIGN"))
                std::tie(local_success, local_message, local_line) = parse_ALIGN(start, last);
            else if (boost::iequals(text_dot, "BLOCK"))
                std::tie(local_success, local_message, local_line) = parse_BLOCK(start, last);
            else if (boost::iequals(text_dot, "BURN")) {
                std::tie(local_success, local_message, local_line) = parse_BURN(start, last);
                if (local_success)
                    burn_count++;
                if (local_success && local_symbol) {
                    local_success = false;
                    local_message = ";ERROR: .BURN does not support symbol declaration.";
                }
            } else if (boost::iequals(text_dot, "BYTE"))
                std::tie(local_success, local_message, local_line) = parse_BYTE(start, last);
            else if (boost::iequals(text_dot, "END")) {
                std::tie(local_success, local_message, local_line) = parse_END(start, last);
                if (local_success)
                    end_count++;
                if (local_success && local_symbol) {
                    local_success = false;
                    local_message = ";ERROR: .END does not support symbol declaration.";
                }
            } else if (boost::iequals(text_dot, "EQUATE")) {
                std::tie(local_success, local_message, local_line) = parse_EQUATE(start, last);
                if (!local_success) {
                }                         // Don't try fixup EQUATE lines that are clearly long.
                else if (!local_symbol) { // EQUATE lines need a symbol declaration.
                    local_success = false;
                    local_message = ";ERROR: .EQUATE requires a symbol declaration.";
                } else {
                    auto arg = std::dynamic_pointer_cast<ir::dot_equate<uint16_t>>(local_line)->argument->value();
                    auto sym_value = std::make_shared<symbol::value_const<uint16_t>>(arg);
                    local_symbol->value = sym_value;
                    assert(local_symbol->value->type() != symbol::Type::kEmpty);
                }
            } else if (boost::iequals(text_dot, "EXPORT")) {
                std::tie(local_success, local_message, local_line) = parse_EXPORT(start, last, symbol_table);
                if (local_success && local_symbol) {
                    local_success = false;
                    local_message = ";ERROR: .EXPORT does not support symbol declaration.";
                }
            } else if (boost::iequals(text_dot, "INPUT")) {
                std::tie(local_success, local_message, local_line) = parse_INPUT(start, last, symbol_table);
                if (!local_success) {
                } // Don't try and fix any errors, but suppress adding additional errors if present.
                else if (local_symbol) {
                    local_success = false;
                    local_message = ";ERROR: .INPUT does not support symbol declaration.";
                }
            } else if (boost::iequals(text_dot, "OUTPUT")) {
                std::tie(local_success, local_message, local_line) = parse_OUTPUT(start, last, symbol_table);
                if (!local_success) {
                } // Don't try and fix any errors, but suppress adding additional errors if present.
                else if (local_symbol) {
                    local_success = false;
                    local_message = ";ERROR: .OUTPUT does not support symbol declaration.";
                }
            } else if (boost::iequals(text_dot, "SCALL")) {
                std::tie(local_success, local_message, local_line) =
                    parse_SCALL(start, last, symbol_table, target->macro_registry);
                if (local_success && local_symbol) {
                    local_success = false;
                    local_message = ";ERROR: .SCALL does not support symbol declaration.";
                }
            } else if (boost::iequals(text_dot, "USCALL")) {
                std::tie(local_success, local_message, local_line) =
                    parse_USCALL(start, last, symbol_table, target->macro_registry);
                if (local_success && local_symbol) {
                    local_success = false;
                    local_message = ";ERROR: .USCALL does not support symbol declaration.";
                }
            } else if (boost::iequals(text_dot, "WORD"))
                std::tie(local_success, local_message, local_line) = parse_WORD(start, last, symbol_table);
            else {
                std::tie(local_success, local_message) = std::make_tuple(false, ";ERROR: Invalid dot command.");
            }

            success &= local_success;
            if (!local_success) {
                target->message_resolver->log_message(section, index, {masm::message_type::kError, local_message});
                continue;
            } else if (local_symbol)
                local_line->symbol_entry = local_symbol;

            // Insert symbol declaration if present.
        } else if (auto [match_macro, _3, text_macro] = masm::frontend::match(start, last, macro, true); match_macro) {

            // Extract the args of the macro.
            auto macro = target->macro_registry->macro(text_macro);

            // Preprocessor already did all the hard work of parsing macros so we don't have to.
            // All we have to do is match on (parent, line number) pairs.
            auto skip = 2 * macro->arg_count - 1;
            if (skip > 0)
                start += skip;

            auto macro_line = std::make_shared<ir::macro_invocation<uint16_t>>();

            // Find the top level section that contains the current section.
            std::shared_ptr<ir::section::top_level_section<uint16_t>> ptr;
            if (ptr = std::dynamic_pointer_cast<ir::section::top_level_section<uint16_t>>(section); ptr)
                ;
            else if (auto as_macro = std::dynamic_pointer_cast<ir::section::macro_subsection<uint16_t>>(section);
                     as_macro) {

                ptr = as_macro->containing_section.lock();
            }

            // Find the macro subsection which this invocation describes.
            for (auto [_, macro_section] : ptr->index_to_macro) {
                if (macro_section->headers.name == text_macro
                    // Make sure that (parent, line number) pair matches the selected section.
                    && macro_section->direct_parent.lock() == section && macro_section->line_number == index) {
                    macro_line->macro = macro_section;
                    break;
                }
            }

            success = macro_line->macro != nullptr;
            local_line = macro_line;
            // Insert symbol declaration if present.
            if (local_symbol)
                local_line->symbol_entry = local_symbol;

        }
        // We had an error, let's try and provide helpful messages for what went wrong.
        else {
            // Unexpected symbol declaration.
            if (local_symbol != nullptr) {
                target->message_resolver->log_message(
                    section, index, {masm::message_type::kError, ";ERROR: Unexpected symbol declaration."});
            }
            // Unexpected end of line.
            else if (auto [match_empty, _1, _2] = masm::frontend::match(start, last, empty, true); match_empty) {
                target->message_resolver->log_message(section, index,
                                                      {masm::message_type::kError, ";ERROR: Unexpected EOL."});
            }
            target->message_resolver->log_message(section, index,
                                                  {masm::message_type::kError, ";ERROR: General parsing error."});
            success = false;
            continue;
        }

        if (auto [match_comment, _1, text_comment] = masm::frontend::match(start, last, comment, true); match_comment) {
            // Add comment to local line.
            local_line->comment = text_comment;
        }

        // ERROR! Line didn't end in a /n.
        if (auto [match_empty, _2, text_what] = masm::frontend::match(start, last, empty, true); !match_empty) {
            target->message_resolver->log_message(
                section, index, {masm::message_type::kError, fmt::format(";ERROR: Expected EOL, got {}", text_what)});
            success = false;
            continue;
        }
        local_line->source_line = index;
        ir_lines.emplace_back(local_line);
    }

    section->ir_lines = ir_lines;
    return success;
}

std::tuple<bool, std::string, asmb::pep10::parser::arg_pointer_t>
asmb::pep10::parser::parse_operand(masm::frontend::token_type token, std::string value,
                                   symbol_table_pointer_t symbol_table) {
    char *end = nullptr;
    long long as_long = 0;
    switch (token) {
    case masm::frontend::token_type::kIdentifier:
        assert(symbol_table);
        if (!valid_symbol_name(value))
            return {false, fmt::format(";ERROR: Invalid symbol \"{}\"", value), nullptr};
        // TODO: Ban using mnemonics as symbols.
        else {
            auto symbol = symbol_table->reference(value);
            auto arg = std::make_shared<ir::symbol_ref_argument<uint16_t>>(symbol);
            return {true, "", arg};
        }
    case masm::frontend::token_type::kCharConstant:
        // TODO: Range check values.
        if (masm::byte_string_length(value) <= 1) {
            return {true, "", std::make_shared<ir::char_argument<uint16_t>>(value)};
        } else
            return {false, ";ERROR: Character literal may only be one byte.", nullptr};
    case masm::frontend::token_type::kStrConstant:
        // TODO: Range check values.
        if (masm::byte_string_length(value) <= 2) {
            return {true, "", std::make_shared<ir::string_argument<uint16_t>>(value)};
        } else {
            return {true, "", std::make_shared<ir::ascii_argument<uint16_t>>(value, 0xffff)};
        }
    case masm::frontend::token_type::kDecConstant:
        // TODO: Range check values.
        as_long = strtol(value.data(), &end, 10);
        if (end != &*value.end())
            return {false, ";ERROR: Not a valid dec constant", nullptr};
        return {true, "", std::make_shared<ir::dec_argument<uint16_t>>(as_long)};
    case masm::frontend::token_type::kHexConstant:
        // TODO: Range check values.
        as_long = strtol(value.data(), &end, 16);
        if (end != &*value.end())
            return {false, ";ERROR: Not a valid dec constant", nullptr};
        return {true, "", std::make_shared<ir::hex_argument<uint16_t>>(as_long)};
    default:
        return {false, {}, nullptr};
    }
}

std::tuple<bool, std::string, asmb::pep10::parser::ir_pointer_t>
asmb::pep10::parser::parse_unary(token_iterator_t &start, const token_iterator_t &last,
                                 isa::pep10::instruction_mnemonic mn) {
    auto ret_val = std::make_shared<asmb::pep10::unary_instruction>();
    ret_val->mnemonic = mn;
    return {true, {}, ret_val};
}

std::tuple<bool, std::string, asmb::pep10::parser::ir_pointer_t>
asmb::pep10::parser::parse_nonunary(token_iterator_t &start, const token_iterator_t &last,
                                    symbol_table_pointer_t symbol_table, isa::pep10::instruction_mnemonic mn) {
    using token_class_t = const std::set<masm::frontend::token_type>;
    static const token_class_t comma = {masm::frontend::token_type::kComma};
    static const token_class_t identifier = {masm::frontend::token_type::kIdentifier};
    static const token_class_t arg = {masm::frontend::token_type::kIdentifier, masm::frontend::token_type::kHexConstant,
                                      masm::frontend::token_type::kDecConstant,
                                      masm::frontend::token_type::kStrConstant,
                                      masm::frontend::token_type::kCharConstant};

    auto ret_val = std::make_shared<asmb::pep10::nonunary_instruction>();
    ret_val->mnemonic = mn;
    if (auto [match_arg, token_arg, text_arg] = masm::frontend::match(start, last, arg, true); !match_arg) {
    } else if (auto [success_operand, err_operand, operand_value] = parse_operand(token_arg, text_arg, symbol_table);
               !success_operand) {
        return {false, err_operand, nullptr};
    } else if (auto [match_comma, _1, _2] = masm::frontend::match(start, last, comma, true);
               requires_addr_mode(mn) && !match_comma) {
        return {false, ";ERROR: mnemonic requires addressing mode.", nullptr};
    } else if (auto [match_addr, _1, text_addr] = masm::frontend::match(start, last, identifier, true);
               requires_addr_mode(mn) && !match_addr) {
        return {false, ";ERROR: Invalid addressing mode.", nullptr};
    } else if (auto parsed = parse_addr_mode(text_addr),
               // If an addr mode is required, use the parsed addr mode.
               // If an addr mode isn't required, then substitute invalid for I.
               // Dear future me: I'm sorry for the nested ternary.
               addr = requires_addr_mode(mn)
                          ? parsed
                          : (parsed == isa::pep10::addressing_mode::INVALID ? isa::pep10::addressing_mode::I : parsed);
               !addr) {
        return {false, ";ERROR: Invalid addressing mode.", nullptr};
    } else if (!allowed_addressing_mode(mn, addr.value())) {
        return {false, fmt::format(";ERROR: Illegal addressing mode \"{}\"", isa::pep10::as_string(addr.value())),
                nullptr};
    }
    // TODO: Explicitly enumerate allowed range in error.
    else if (!operand_value->fits_in(2))
        return {false, ";ERROR: Operand must fit in 2 bytes.", nullptr};
    else {
        ret_val->argument = operand_value;
        ret_val->addressing_mode = addr.value();
    }
    return {true, {}, ret_val};
}

asmb::pep10::parser::ir_pointer_t asmb::pep10::parser::parse_macro_invocation(token_iterator_t &start,
                                                                              const token_iterator_t &last) {
    using token_class_t = const std::set<masm::frontend::token_type>;

    return nullptr;
}

std::tuple<bool, std::string, asmb::pep10::parser::ir_pointer_t>
asmb::pep10::parser::parse_ASCII(token_iterator_t &start, const token_iterator_t &last) {
    using token_class_t = const std::set<masm::frontend::token_type>;
    static const token_class_t arg = {masm::frontend::token_type::kStrConstant};

    auto ret_val = std::make_shared<ir::dot_ascii<uint16_t>>();
    if (auto [match_arg, _, text_arg] = masm::frontend::match(start, last, arg, true); !match_arg) {
        return {false, ";ERROR: .ASCII requires a string argument.", nullptr};
    } else {
        ret_val->argument = std::make_shared<ir::ascii_argument<uint16_t>>(text_arg, 0xFFFF);
        return {true, "", ret_val};
    };
}

std::tuple<bool, std::string, asmb::pep10::parser::ir_pointer_t>
asmb::pep10::parser::parse_ALIGN(token_iterator_t &start, const token_iterator_t &last) {
    using token_class_t = const std::set<masm::frontend::token_type>;
    static const token_class_t arg = {masm::frontend::token_type::kDecConstant,
                                      masm::frontend::token_type::kHexConstant};

    auto ret_val = std::make_shared<ir::dot_align<uint16_t>>();
    if (auto [match_arg, token_arg, text_arg] = masm::frontend::match(start, last, arg, true); !match_arg) {
        return {false, ";ERROR: .ALIGN requires a (hexa)decimal argument.", nullptr};
    } else if (auto [valid_operand, err_msg, argument] = parse_operand(token_arg, text_arg, nullptr); !valid_operand) {
        return {false, err_msg, nullptr};
    } else if (auto v = argument->value(); v % 2 != 0 && v % 4 != 0 && v % 8 != 0) {
        return {false, ";ERROR: .ALIGN argument must be in {2,4,8}.", nullptr};
    } else {
        ret_val->argument = argument;
        return {true, "", ret_val};
    };
}

std::tuple<bool, std::string, asmb::pep10::parser::ir_pointer_t>
asmb::pep10::parser::parse_BLOCK(token_iterator_t &start, const token_iterator_t &last) {
    using token_class_t = const std::set<masm::frontend::token_type>;
    static const token_class_t arg = {
        masm::frontend::token_type::kDecConstant,
        masm::frontend::token_type::kHexConstant,
    };

    auto ret_val = std::make_shared<ir::dot_block<uint16_t>>();
    if (auto [match_arg, token_arg, text_arg] = masm::frontend::match(start, last, arg, true); !match_arg) {
        return {false, ";ERROR: .BLOCK requires a literal argument.", nullptr};
    } else if (auto [valid_operand, err_msg, argument] = parse_operand(token_arg, text_arg, nullptr); !valid_operand) {
        return {false, err_msg, nullptr};
    } else if (!argument->fits_in(2)) {
        return {false, ";ERROR: Operand must be smaller than 2 bytes.", nullptr};
    } else {
        ret_val->argument = argument;
        return {true, "", ret_val};
    };
}

std::tuple<bool, std::string, asmb::pep10::parser::ir_pointer_t>
asmb::pep10::parser::parse_BURN(token_iterator_t &start, const token_iterator_t &last) {
    using token_class_t = const std::set<masm::frontend::token_type>;
    static const token_class_t arg = {
        masm::frontend::token_type::kHexConstant,
    };

    auto ret_val = std::make_shared<ir::dot_burn<uint16_t>>();
    if (auto [match_arg, token_arg, text_arg] = masm::frontend::match(start, last, arg, true); !match_arg) {
        return {false, ";ERROR: .BURN requires a hex argument.", nullptr};
    } else if (auto [valid_operand, err_msg, argument] = parse_operand(token_arg, text_arg, nullptr); !valid_operand) {
        return {false, err_msg, nullptr};
    } else if (!argument->fits_in(2)) {
        return {false, ";ERROR: Operand must be smaller than 2 bytes.", nullptr};
    } else {
        ret_val->argument = argument;
        return {true, "", ret_val};
    };
}

std::tuple<bool, std::string, asmb::pep10::parser::ir_pointer_t>
asmb::pep10::parser::parse_BYTE(token_iterator_t &start, const token_iterator_t &last) {
    using token_class_t = const std::set<masm::frontend::token_type>;
    static const token_class_t arg = {
        masm::frontend::token_type::kDecConstant, masm::frontend::token_type::kHexConstant,
        masm::frontend::token_type::kStrConstant, masm::frontend::token_type::kCharConstant};

    auto ret_val = std::make_shared<ir::dot_byte<uint16_t>>();
    if (auto [match_arg, token_arg, text_arg] = masm::frontend::match(start, last, arg, true); !match_arg) {
        return {false, ";ERROR: .BYTE requires a literal argument.", nullptr};
    } else if (auto [valid_operand, err_msg, argument] = parse_operand(token_arg, text_arg, nullptr); !valid_operand) {
        return {false, err_msg, nullptr};
    } else if (!argument->fits_in(1)) {
        return {false, ";ERROR: Operand must be smaller than 1 byte.", nullptr};
    } else {
        ret_val->argument = argument;
        return {true, "", ret_val};
    };
}

std::tuple<bool, std::string, asmb::pep10::parser::ir_pointer_t>
asmb::pep10::parser::parse_END(token_iterator_t &start, const token_iterator_t &last) {
    return {true, {}, std::make_shared<ir::dot_end<uint16_t>>()};
}

std::tuple<bool, std::string, asmb::pep10::parser::ir_pointer_t>
asmb::pep10::parser::parse_EQUATE(token_iterator_t &start, const token_iterator_t &last) {
    using token_class_t = const std::set<masm::frontend::token_type>;
    static const token_class_t arg = {
        masm::frontend::token_type::kDecConstant, masm::frontend::token_type::kHexConstant,
        masm::frontend::token_type::kStrConstant, masm::frontend::token_type::kCharConstant};

    auto ret_val = std::make_shared<ir::dot_equate<uint16_t>>();
    if (auto [match_arg, token_arg, text_arg] = masm::frontend::match(start, last, arg, true); !match_arg) {
        return {false, ";ERROR: .EQUATE requires a literal argument.", nullptr};
    } else if (auto [valid_operand, err_msg, argument] = parse_operand(token_arg, text_arg, nullptr); !valid_operand) {
        return {false, err_msg, nullptr};
    } else if (!argument->fits_in(2)) {
        return {false, ";ERROR: Operand must be smaller than 2 bytes.", nullptr};
    } else {
        ret_val->argument = argument;
        return {true, "", ret_val};
    };
}

std::tuple<bool, std::string, asmb::pep10::parser::ir_pointer_t>
asmb::pep10::parser::parse_EXPORT(token_iterator_t &start, const token_iterator_t &last,
                                  symbol_table_pointer_t symbol_table) {
    using token_class_t = const std::set<masm::frontend::token_type>;
    static const token_class_t arg = {masm::frontend::token_type::kIdentifier};

    auto ret_val = std::make_shared<asmb::pep10::dot_export<uint16_t>>();
    if (auto [match_arg, token_arg, text_arg] = masm::frontend::match(start, last, arg, true); !match_arg) {
        return {false, ";ERROR: .EXPORT requires a symbolic argument.", nullptr};
    } else if (auto [valid_operand, err_msg, argument] = parse_operand(token_arg, text_arg, symbol_table);
               !valid_operand) {
        return {false, err_msg, nullptr};
    } else {
        auto as_ref = std::dynamic_pointer_cast<ir::symbol_ref_argument<uint16_t>>(argument);
        symbol_table->mark_global(as_ref->symbol_value()->name);
        assert(as_ref);
        ret_val->argument = as_ref;
        return {true, "", ret_val};
    }
}

std::tuple<bool, std::string, asmb::pep10::parser::ir_pointer_t>
asmb::pep10::parser::parse_INPUT(token_iterator_t &start, const token_iterator_t &last,
                                 symbol_table_pointer_t symbol_table) {
    using token_class_t = const std::set<masm::frontend::token_type>;
    static const token_class_t arg = {masm::frontend::token_type::kIdentifier};

    auto ret_val = std::make_shared<ir::dot_input<uint16_t>>();
    if (auto [match_arg, token_arg, text_arg] = masm::frontend::match(start, last, arg, true); !match_arg) {
        return {false, ";ERROR: .INPUT requires a symbolic argument.", nullptr};
    } else if (auto [valid_operand, err_msg, argument] = parse_operand(token_arg, text_arg, symbol_table);
               !valid_operand) {
        return {false, err_msg, nullptr};
    } else {
        auto as_ref = std::dynamic_pointer_cast<ir::symbol_ref_argument<uint16_t>>(argument);
        ret_val->argument = as_ref;
        return {true, "", ret_val};
    }
}

std::tuple<bool, std::string, asmb::pep10::parser::ir_pointer_t>
asmb::pep10::parser::parse_OUTPUT(token_iterator_t &start, const token_iterator_t &last,
                                  symbol_table_pointer_t symbol_table) {
    using token_class_t = const std::set<masm::frontend::token_type>;
    static const token_class_t arg = {masm::frontend::token_type::kIdentifier};

    auto ret_val = std::make_shared<ir::dot_output<uint16_t>>();
    if (auto [match_arg, token_arg, text_arg] = masm::frontend::match(start, last, arg, true); !match_arg) {
        return {false, ";ERROR: .OUTPUT requires a symbolic argument.", nullptr};
    } else if (auto [valid_operand, err_msg, argument] = parse_operand(token_arg, text_arg, symbol_table);
               !valid_operand) {
        return {false, err_msg, nullptr};
    } else {
        auto as_ref = std::dynamic_pointer_cast<ir::symbol_ref_argument<uint16_t>>(argument);
        ret_val->argument = as_ref;
        return {true, "", ret_val};
    }
}

std::tuple<bool, std::string, asmb::pep10::parser::ir_pointer_t>
asmb::pep10::parser::parse_SCALL(token_iterator_t &start, const token_iterator_t &last,
                                 symbol_table_pointer_t symbol_table, macro_registry_pointer_t registry) {
    using token_class_t = const std::set<masm::frontend::token_type>;
    static const token_class_t arg = {masm::frontend::token_type::kIdentifier};

    auto ret_val = std::make_shared<asmb::pep10::dot_scall<uint16_t>>();
    if (auto [match_arg, token_arg, text_arg] = masm::frontend::match(start, last, arg, true); !match_arg) {
        return {false, ";ERROR: .SCALL requires a symbolic argument.", nullptr};
    } else if (auto [valid_operand, err_msg, argument] = parse_operand(token_arg, text_arg, symbol_table);
               !valid_operand) {
        return {false, err_msg, nullptr};
    } else {
        registry->register_nonunary_system_call(text_arg, "@{0} 2\nLDWT {0},i\nSCALL $1, $2\n.END\n");
        auto as_ref = std::dynamic_pointer_cast<ir::symbol_ref_argument<uint16_t>>(argument);
        ret_val->argument = as_ref;
        return {true, "", ret_val};
    };
}

std::tuple<bool, std::string, asmb::pep10::parser::ir_pointer_t>
asmb::pep10::parser::parse_USCALL(token_iterator_t &start, const token_iterator_t &last,
                                  symbol_table_pointer_t symbol_table, macro_registry_pointer_t registry) {
    using token_class_t = const std::set<masm::frontend::token_type>;
    static const token_class_t arg = {masm::frontend::token_type::kIdentifier};

    auto ret_val = std::make_shared<asmb::pep10::dot_uscall<uint16_t>>();
    if (auto [match_arg, token_arg, text_arg] = masm::frontend::match(start, last, arg, true); !match_arg) {
        return {false, ";ERROR: .USCALL requires a symbolic argument.", nullptr};
    } else if (auto [valid_operand, err_msg, argument] = parse_operand(token_arg, text_arg, symbol_table);
               !valid_operand) {
        return {false, err_msg, nullptr};
    } else {
        registry->register_unary_system_call(text_arg, "@{0} 0\nLDWT {0},i\nUSCALL\n.END\n");
        auto as_ref = std::dynamic_pointer_cast<ir::symbol_ref_argument<uint16_t>>(argument);
        ret_val->argument = as_ref;
        return {true, "", ret_val};
    };
}

std::tuple<bool, std::string, asmb::pep10::parser::ir_pointer_t>
asmb::pep10::parser::parse_WORD(token_iterator_t &start, const token_iterator_t &last,
                                symbol_table_pointer_t symbol_table) {
    using token_class_t = const std::set<masm::frontend::token_type>;
    static const token_class_t arg = {
        masm::frontend::token_type::kDecConstant, masm::frontend::token_type::kHexConstant,
        masm::frontend::token_type::kStrConstant, masm::frontend::token_type::kCharConstant,
        masm::frontend::token_type::kIdentifier};

    auto ret_val = std::make_shared<ir::dot_word<uint16_t>>();
    if (auto [match_arg, token_arg, text_arg] = masm::frontend::match(start, last, arg, true); !match_arg) {
        return {false, ";ERROR: .WORD requires a literal argument.", nullptr};
    } else if (auto [valid_operand, err_msg, argument] = parse_operand(token_arg, text_arg, symbol_table);
               !valid_operand) {
        return {false, err_msg, nullptr};
    } else if (!argument->fits_in(2)) {
        return {false, ";ERROR: Operand must be smaller than 2 bytes.", nullptr};
    } else {
        ret_val->argument = argument;
        return {true, "", ret_val};
    };
}