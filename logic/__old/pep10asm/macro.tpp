#include "macro.hpp"
#include <boost/algorithm/string.hpp>

template <typename address_size_t> masm::ir::macro_invocation<address_size_t>::macro_invocation() {
    this->emits_object_code = true;
}
template <typename address_size_t>
masm::ir::macro_invocation<address_size_t>::macro_invocation(const masm::ir::macro_invocation<address_size_t> &other) {}

template <typename address_size_t>
masm::ir::macro_invocation<address_size_t> &
masm::ir::macro_invocation<address_size_t>::operator=(masm::ir::macro_invocation<address_size_t> other) {
    swap(*this, other);
    return *this;
}

template <typename address_size_t>
std::shared_ptr<masm::ir::linear_line<address_size_t>> masm::ir::macro_invocation<address_size_t>::clone() const {
    return std::make_shared<macro_invocation<address_size_t>>(*this);
}

template <typename address_size_t> masm::ir::ByteType masm::ir::macro_invocation<address_size_t>::bytes_type() const {
    return masm::ir::ByteType::kData;
}

template <typename address_size_t>
std::string masm::ir::macro_invocation<address_size_t>::generate_listing_string() const {
    auto args = boost::algorithm::join(macro->macro_args, ", ");
    auto temp = fmt::format("{:<6} {:<6};@{} {}", " ", " ", macro->header.name, args);
    if (this->comment) 
        auto temp = fmt::format("{:<13}{}", " ", this->get_formatted_comment());
    for (auto line : macro->body_ir.value().ir_lines) {
        // Don't include a macros .END directive. This would make the listing confusing.
        if (auto as_end = std::dynamic_pointer_cast<masm::ir::dot_end<address_size_t>>(line); as_end)
            continue;
        temp.append(fmt::format("\n{}", line->generate_listing_string()));
    }
    const auto formatted = fmt::vformat("\n{:<13};End @{} {}", fmt::make_format_args(" ", macro->header.name, args));
    temp.append(formatted);

    return temp;
}

template <typename address_size_t>
std::string masm::ir::macro_invocation<address_size_t>::generate_source_string() const {
    std::string symbol_string;
    if (this->symbol_entry != nullptr) {
        symbol_string = this->symbol_entry->name + ":";
    }
    auto macro_name = macro->header.name;
    auto operand_string = boost::algorithm::join(macro->macro_args, ", ");
    return fmt::format("{:<9}{:<8}{:<12}{}", symbol_string, macro_name, operand_string, this->get_formatted_comment());
}

template <typename address_size_t>
address_size_t masm::ir::macro_invocation<address_size_t>::object_code_bytes() const {
    address_size_t bytes = 0;
    for (auto line : macro->body_ir.value().ir_lines) {
        bytes += line->object_code_bytes();
    }
    return bytes;
}

template <typename address_size_t>
void masm::ir::macro_invocation<address_size_t>::append_object_code(std::vector<uint8_t> &bytes) const {
    if (!this->emits_object_code)
        return;
    for (const auto &line : macro->body_ir.value().ir_lines) {
        line->append_object_code(bytes);
    }
}
