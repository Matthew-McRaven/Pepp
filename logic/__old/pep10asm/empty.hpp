#pragma once

#include <memory>
#include "base.hpp"

namespace masm::ir {

template <typename address_size_t>
class comment_line: public masm::ir::linear_line<address_size_t>
{
public:
    comment_line() = default;
    ~comment_line() override = default;
    comment_line(const comment_line& other);
    comment_line& operator=(comment_line other);
    std::shared_ptr<linear_line<address_size_t>> clone() const override;

    // linear_line interface
    std::string generate_listing_string() const override;
    std::string generate_source_string() const override;
    void append_object_code(std::vector<uint8_t>& code) const override;
    
    friend void swap(comment_line& first, comment_line& second)
    {
        using std::swap;
        swap(static_cast<linear_line<address_size_t>&>(first), static_cast<linear_line<address_size_t>&>(second));
    }
};

template <typename address_size_t>
class blank_line: public masm::ir::linear_line<address_size_t>
{
public:
    blank_line() = default;
    ~blank_line() override = default;
    blank_line(const blank_line& other);
    blank_line& operator=(blank_line other);
    std::shared_ptr<linear_line<address_size_t>> clone() const override;

    // linear_line interface
    std::string generate_listing_string() const override;
    std::string generate_source_string() const override;
    void append_object_code(std::vector<uint8_t>& code) const override;

    friend void swap(blank_line& first, blank_line& second)
    {
        using std::swap;
        swap(static_cast<linear_line<address_size_t>&>(first), static_cast<linear_line<address_size_t>&>(second));
    }
};
}; // End namespace masm::ir

#include "empty.tpp"
