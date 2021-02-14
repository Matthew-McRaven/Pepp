#pragma once

#include <optional>

#include "isa/pep10/defs.hpp"
#include "masm/ir/base.hpp"
#include "masm/ir/args.hpp"
namespace asmb::pep10 {

class unary_instruction: public masm::ir::linear_line<uint16_t>
{
public:
    unary_instruction();
    ~unary_instruction() override = default;
    unary_instruction(const unary_instruction& other);
    unary_instruction& operator=(unary_instruction other);
    std::shared_ptr<masm::ir::linear_line<uint16_t>> clone() const override;

    // Get the assembler listing, which is memaddress + object code + sourceLine.
    std::string generate_listing_string() const override;
    // Returns the properly formatted source line.
    std::string generate_source_string() const override;
    uint16_t object_code_bytes() const override;
    void append_object_code(std::vector<uint8_t>& code) const override;

    bool is_code() const override { return true;}

    friend void swap(unary_instruction& first, unary_instruction& second)
    {
        using std::swap;
        swap(static_cast<masm::ir::linear_line<uint16_t>&>(first), 
            static_cast<masm::ir::linear_line<uint16_t>&>(second)
        );
        swap(first.mnemonic, second.mnemonic);
    }
    
    isa::pep10::instruction_mnemonic mnemonic;
};

class nonunary_instruction: public masm::ir::linear_line<uint16_t>
{
public:
    nonunary_instruction();
    ~nonunary_instruction() override = default;
    nonunary_instruction(const nonunary_instruction& other);
    nonunary_instruction& operator=(nonunary_instruction other);
    std::shared_ptr<masm::ir::linear_line<uint16_t>> clone() const override;

    // Get the assembler listing, which is memaddress + object code + sourceLine.
    std::string generate_listing_string() const override;
    // Returns the properly formatted source line.
    std::string generate_source_string() const override;
    uint16_t object_code_bytes() const override;
    void append_object_code(std::vector<uint8_t>& code) const override;
    std::optional<std::shared_ptr<const symbol::entry<uint16_t>>> symbolic_operand() const override;

    bool is_code() const override { return true;}

    friend void swap(nonunary_instruction& first, nonunary_instruction& second)
    {
        using std::swap;
        swap(static_cast<masm::ir::linear_line<uint16_t>&>(first), 
            static_cast<masm::ir::linear_line<uint16_t>&>(second)
        );
        swap(first.mnemonic, second.mnemonic);
        swap(first.addressing_mode, second.addressing_mode);
        swap(first.argument, second.argument);
    }

    isa::pep10::instruction_mnemonic mnemonic;
    isa::pep10::addressing_mode addressing_mode;
    std::shared_ptr<masm::ir::lir_argument<uint16_t>> argument = nullptr;
};
} // End namespace asmb::pep10