#include "empty.hpp"

#include <fmt/core.h>

template <typename address_size_t>
masm::ir::comment_line<address_size_t>::comment_line(const masm::ir::comment_line<address_size_t>& other)
{

}

template <typename address_size_t>
masm::ir::comment_line<address_size_t> &masm::ir::comment_line<address_size_t>::operator=(
	masm::ir::comment_line<address_size_t> other)
{
    swap(*this, other);
    return *this;
}


template <typename address_size_t>
std::shared_ptr<masm::ir::linear_line<address_size_t>> masm::ir::comment_line<address_size_t>::clone() const
{
    return std::make_shared<comment_line<address_size_t> >(*this);
}

template <typename address_size_t>
std::string masm::ir::comment_line<address_size_t>::generate_listing_string() const
{
    return fmt::format("             {}", generate_source_string());
}

template <typename address_size_t>
std::string masm::ir::comment_line<address_size_t>::generate_source_string() const
{
	// Unwrap variant if it is a string, and return it.
    if(auto pval = std::get_if<std::string>(&this->comment)) return std::string{*pval};
	else return "";
}

template <typename address_size_t>
masm::ir::blank_line<address_size_t>::blank_line(const masm::ir::blank_line<address_size_t>& other)
{

}

template <typename address_size_t>
masm::ir::blank_line<address_size_t> &masm::ir::blank_line<address_size_t>::operator=(
	masm::ir::blank_line<address_size_t> other)
{
    swap(*this, other);
    return *this;
}


template <typename address_size_t>
std::shared_ptr<masm::ir::linear_line<address_size_t>> masm::ir::blank_line<address_size_t>::clone() const
{
    return std::make_shared<blank_line<address_size_t> >(*this);
}

template <typename address_size_t>
std::string masm::ir::blank_line<address_size_t>::generate_listing_string() const
{
    return {};
}

template <typename address_size_t>
std::string masm::ir::blank_line<address_size_t>::generate_source_string() const
{
	return {};
}
