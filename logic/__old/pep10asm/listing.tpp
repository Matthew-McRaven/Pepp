#include "listing.hpp"

#include <boost/algorithm/string.hpp>
#include <fmt/core.h>

#include "utils/format.hpp"
template <typename addr_size_t>
std::string masm::utils::generate_listing(std::shared_ptr<masm::elf::top_level_section<addr_size_t>> &image) {
    std::stringstream out_stream;
    out_stream << "-------------------------------------------------------------------------------" << std::endl;
    out_stream << "       Object" << std::endl;
    out_stream << "Addr   code   Symbol   Mnemon  Operand     Comment" << std::endl;
    out_stream << "-------------------------------------------------------------------------------" << std::endl;

    for (const auto &line : image->body_ir->ir_lines) {
        out_stream << line->generate_listing_string() << std::endl;
    }
    out_stream << "-------------------------------------------------------------------------------" << std::endl;
    out_stream << "Symbol Table" << std::endl;
    out_stream << "--------------------------------" << std::endl;
    out_stream << "Symbol    Value     Symbol    Value" << std::endl;
    out_stream << "--------------------------------" << std::endl;
    out_stream << symbol::symbol_table_listing<uint16_t>(image->symbol_table);
    out_stream << "--------------------------------" << std::endl;
    return out_stream.str();
}

template <typename addr_size_t>
std::vector<uint8_t> masm::utils::get_bytecode(std::shared_ptr<masm::elf::top_level_section<addr_size_t>> &image) {
    std::vector<uint8_t> object_code;
    for (const auto &line : image->body_ir->ir_lines)
        line->append_object_code(object_code);
    return object_code;
}

template <typename addr_size_t>
std::string masm::utils::generate_formatted_bytecode(std::shared_ptr<masm::elf::top_level_section<addr_size_t>> &image,
                                                     uint8_t bytes_per_line) {
    assert(bytes_per_line > 0);

    auto object_code = get_bytecode(image);
    return ::utils::bytes_to_hex_string(object_code, bytes_per_line, true);
}

template <typename addr_size_t>
std::string masm::utils::generate_pretty_object_code(std::shared_ptr<masm::elf::top_level_section<addr_size_t>> &image,
                                                     uint8_t base, bool include_comment) {
    assert(base > 0);
    std::stringstream ss;
    std::size_t maxLen = 1;
    for (const auto &line : image->body_ir->ir_lines) {
        std::vector<uint8_t> object_code;
        line->append_object_code(object_code);
        maxLen = std::max(object_code.size(), maxLen);
        auto formatted_bytes = ::utils::bytes_to_nbit_string(object_code, 255, base, 4, false, base == 2);
        auto addr_string =
            line->bytes_type() == masm::ir::ByteType::kNoBytes ? "    " : fmt::format("{:04X}", line->base_address());
        // std::cout << addr_string << std::endl;
        auto fmt = fmt::format("{{:4}} {} {{}}", "{:" + std::to_string(maxLen) + "}");
        auto formatted = fmt::vformat(fmt, fmt::make_format_args(addr_string, formatted_bytes,
                                                                 include_comment ? line->get_formatted_comment() : ""));
        boost::algorithm::trim_right(formatted);
        // std::cout << fmt << std::endl;
        if (formatted.size())
            ss << formatted << "\n";
    }

    return ss.str();
}