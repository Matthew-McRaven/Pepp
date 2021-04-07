#pragma once
#include "base.hpp"
namespace masm::ir {
template <typename address_size_t>
class macro_invocation: public linear_line<address_size_t>
{
public:
    macro_invocation();
    ~macro_invocation() override = default;
    macro_invocation(const macro_invocation<address_size_t>& other);
    macro_invocation& operator=(macro_invocation<address_size_t> other);
    std::shared_ptr<linear_line<address_size_t> > clone() const override;

    // Get the assembler listing, which is memaddress + object code + sourceLine.
    std::string generate_listing_string() const override;
    // Returns the properly formatted source line.
    std::string generate_source_string() const override;
    address_size_t object_code_bytes() const override;
    void append_object_code(std::vector<uint8_t>& code) const override;
    
    friend void swap(macro_invocation<address_size_t>& first, macro_invocation<address_size_t>& second)
    {
        using std::swap;
        swap(static_cast<linear_line<address_size_t>&>(first), 
            static_cast<linear_line<address_size_t>&>(second));
        swap(first.macro, second.macro);
    }

    std::shared_ptr<masm::elf::macro_subsection<address_size_t> > macro;
};
}; // End namespace masm::ir
#include "macro.tpp"