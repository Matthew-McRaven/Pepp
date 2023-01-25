#include "ir.hpp"

#include "symbol/entry.hpp"
#include "symbol/table.hpp"
/*
 * Unary Instruction
 */
asmb::pep10::unary_instruction::unary_instruction() : ir::linear_line<uint16_t>() { this->emits_object_code = true; }
asmb::pep10::unary_instruction::unary_instruction(const asmb::pep10::unary_instruction &other)
    : ir::linear_line<uint16_t>(other), mnemonic(other.mnemonic) {}

asmb::pep10::unary_instruction &asmb::pep10::unary_instruction::operator=(asmb::pep10::unary_instruction other) {
    swap(*this, other);
    return *this;
}

std::shared_ptr<ir::linear_line<uint16_t>> asmb::pep10::unary_instruction::clone() const {
    return std::make_shared<unary_instruction>(*this);
}

ir::ByteType asmb::pep10::unary_instruction::bytes_type() const { return ir::ByteType::kCode; }

std::string asmb::pep10::unary_instruction::generate_listing_string() const {
    // Potentially skip codegen
    std::string code_string = "";
    if (this->emits_object_code) {
        // TODO: Get correct mnemonic value.
        code_string = fmt::format("{:02X}", isa::pep10::opcode(mnemonic));
    }

    return fmt::format("{:<6} {:<6} {}", fmt::format("{:04X}", this->base_address()), code_string,
                       generate_source_string());
}

std::string asmb::pep10::unary_instruction::generate_source_string() const {
    std::string symbol_string;
    if (this->symbol_entry != nullptr) {
        symbol_string = this->symbol_entry->name + ":";
    }
    // TODO: Get correct mnemonic string
    auto mnemonic_string = magic_enum::enum_name(this->mnemonic);
    std::string comment = this->comment.value_or("");
    return fmt::format("{:<9}{:<8}{:<12}{}", symbol_string, mnemonic_string, "", this->get_formatted_comment());
}

uint16_t asmb::pep10::unary_instruction::object_code_bytes() const { return 1; }

void asmb::pep10::unary_instruction::append_object_code(std::vector<uint8_t> &bytes) const {
    if (!this->emits_object_code)
        return;
    bytes.emplace_back(isa::pep10::opcode(mnemonic));
}

/*
 * Non-unary Instruction
 */
asmb::pep10::nonunary_instruction::nonunary_instruction() : ir::linear_line<uint16_t>() {
    this->emits_object_code = true;
}
asmb::pep10::nonunary_instruction::nonunary_instruction(const asmb::pep10::nonunary_instruction &other)
    : ir::linear_line<uint16_t>(other), mnemonic(other.mnemonic), addressing_mode(other.addressing_mode),
      argument(other.argument) {}

asmb::pep10::nonunary_instruction &
asmb::pep10::nonunary_instruction::operator=(asmb::pep10::nonunary_instruction other) {
    swap(*this, other);
    return *this;
}

std::shared_ptr<ir::linear_line<uint16_t>> asmb::pep10::nonunary_instruction::clone() const {
    return std::make_shared<nonunary_instruction>(*this);
}

ir::ByteType asmb::pep10::nonunary_instruction::bytes_type() const { return ir::ByteType::kCode; }

std::string asmb::pep10::nonunary_instruction::generate_listing_string() const {
    // Potentially skip codegen
    std::string code_string = "";
    if (this->emits_object_code) {
        code_string = fmt::format("{:02x}{:04X}", isa::pep10::opcode(mnemonic, addressing_mode), argument->value());
    }

    return fmt::format("{:<6} {:<6} {}", fmt::format("{:04X}", this->base_address()), code_string,
                       generate_source_string());
}

std::string asmb::pep10::nonunary_instruction::generate_source_string() const {
    std::string symbol_string;
    if (this->symbol_entry != nullptr) {
        symbol_string = this->symbol_entry->name + ":";
    }

    auto mnemonic_string = magic_enum::enum_name(this->mnemonic);
    auto operand_string = argument->string() + ",";

    // Must make a copy, since addressing_mode is a const, capitalized string.
    auto addr_mode = std::string(magic_enum::enum_name(this->addressing_mode));
    // Lowercase the addressing mode for consistency with Pep/9 (see #395).
    std::transform(addr_mode.begin(), addr_mode.end(), addr_mode.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    operand_string.append(addr_mode);

    return fmt::format("{:<9}{:<8}{:<12}{}", symbol_string, mnemonic_string, operand_string,
                       this->get_formatted_comment());
}

uint16_t asmb::pep10::nonunary_instruction::object_code_bytes() const { return 3; }

void asmb::pep10::nonunary_instruction::append_object_code(std::vector<uint8_t> &bytes) const {
    if (!this->emits_object_code)
        return;
    bytes.emplace_back(isa::pep10::opcode(mnemonic, addressing_mode));
    // Convert argument from 16 bits to 2x8 bits.
    uint16_t arg = argument->value();
    uint8_t hi = (arg >> 8) & 0xff, lo = arg & 0xff;
    bytes.emplace_back(hi);
    bytes.emplace_back(lo);
}

std::optional<std::shared_ptr<const symbol::entry<uint16_t>>>
asmb::pep10::nonunary_instruction::symbolic_operand() const {
    if (auto as_symbolic = std::dynamic_pointer_cast<ir::symbol_ref_argument<uint16_t>>(argument)) {
        return as_symbolic->symbol_value();
    }
    return std::nullopt;
}