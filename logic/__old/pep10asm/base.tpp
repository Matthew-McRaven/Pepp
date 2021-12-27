#include "base.hpp"
template <typename address_size_t>

masm::ir::linear_line<address_size_t>::linear_line()
// No need to init, we already took care of that in definition!
{}

template <typename address_size_t>
masm::ir::linear_line<address_size_t>::linear_line(const linear_line<address_size_t> &other) {
    this->emits_object_code = other.emits_object_code;
    this->address_span = other.address_span;
    this->breakpoint = other.breakpoint;
    this->comment = other.comment;
    this->source_line = other.source_line;
    this->listing_line = other.listing_line;
    this->symbol_entry = other.symbol_entry;
    // this->trace = other.trace;
}

template <typename address_size_t> masm::ir::linear_line<address_size_t>::~linear_line() = default;

template <typename address_size_t> std::string masm::ir::linear_line<address_size_t>::get_formatted_comment() const {
    if (this->comment)
        return ";" + *comment;
    else
        return "";
}