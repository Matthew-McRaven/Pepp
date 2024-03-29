#include "directives.hpp"

#include <fmt/core.h>

#include "masm/conversion.hpp"
#include "asm/symbol/table.hpp"

/*
 * .ADDRSS
 */
template <typename address_size_t>
masm::ir::dot_address<address_size_t>::dot_address(const masm::ir::dot_address<address_size_t> &other) {}

template <typename address_size_t>
masm::ir::dot_address<address_size_t> &
masm::ir::dot_address<address_size_t>::operator=(masm::ir::dot_address<address_size_t> other) {
    swap(*this, other);
    return *this;
}

template <typename address_size_t>
std::shared_ptr<masm::ir::linear_line<address_size_t>> masm::ir::dot_address<address_size_t>::clone() const {
    return std::make_shared<dot_address<address_size_t>>(*this);
}

template <typename address_size_t> masm::ir::ByteType masm::ir::dot_address<address_size_t>::bytes_type() const {
    return masm::ir::ByteType::kData;
}

template <typename address_size_t> std::string masm::ir::dot_address<address_size_t>::generate_listing_string() const {
    // Potentially skip codegen
    std::string code_string = "";
    if (this->emits_object_code) {
        code_string = fmt::format("{:04X}", this->argument->value());
    }

    return fmt::format("{:<6} {:<6} {}", fmt::format("{:04X}", this->base_address()), code_string,
                       generate_source_string());
}

template <typename address_size_t> std::string masm::ir::dot_address<address_size_t>::generate_source_string() const {
    std::string symbol_string;
    if (this->symbol_entry != nullptr) {
        symbol_string = this->symbol_entry->name + ":";
    }
    auto dot_string = ".ADDRSS";
    auto operand_string = argument->string();
    return fmt::format("{:<9}{:<8}{:<12}{}", symbol_string, dot_string, operand_string, this->get_formatted_comment());
}

template <typename address_size_t> address_size_t masm::ir::dot_address<address_size_t>::object_code_bytes() const {
    return 2;
}

template <typename address_size_t>
void masm::ir::dot_address<address_size_t>::append_object_code(std::vector<uint8_t> &bytes) const {
    if (!this->emits_object_code)
        return;
    bytes.emplace_back((argument->value() >> 8) & 0xff);
    bytes.emplace_back(argument->value() & 0xff);
}

template <typename address_size_t>
std::optional<std::shared_ptr<const symbol::entry<address_size_t>>>
masm::ir::dot_address<address_size_t>::symbolic_operand() const {
    // The value of a .addrss instruction is always the value of another symbol.
    return argument.get()->symbol_value();
}

/*
 * .ALIGN
 */
template <typename address_size_t> masm::ir::dot_align<address_size_t>::dot_align() { this->emits_object_code = true; }

template <typename address_size_t>
masm::ir::dot_align<address_size_t>::dot_align(const masm::ir::dot_align<address_size_t> &other) {}

template <typename address_size_t>
masm::ir::dot_align<address_size_t> &
masm::ir::dot_align<address_size_t>::operator=(masm::ir::dot_align<address_size_t> other) {
    swap(*this, other);
    return *this;
}

template <typename address_size_t>
std::shared_ptr<masm::ir::linear_line<address_size_t>> masm::ir::dot_align<address_size_t>::clone() const {
    return std::make_shared<dot_align<address_size_t>>(*this);
}

template <typename address_size_t> masm::ir::ByteType masm::ir::dot_align<address_size_t>::bytes_type() const {
    return masm::ir::ByteType::kData;
}

template <typename address_size_t> std::string masm::ir::dot_align<address_size_t>::generate_listing_string() const {
    // Potentially skip codegen
    std::string code_string = "";
    auto bytes_remaining = this->num_bytes_generated();
    auto bytes_emitted = 0;

    while (this->emits_object_code && (bytes_remaining > 0) && (bytes_emitted < 3)) {
        code_string.append("00");
        ++bytes_emitted;
        --bytes_remaining;
    }

    auto temp = fmt::format("{:<6} {:<6} {}", fmt::format("{:04X}", this->base_address()), code_string,
                            generate_source_string());

    while (bytes_remaining > 0) {
        code_string = "";
        bytes_emitted = 0;
        while (this->emits_object_code && (bytes_remaining > 0) && (bytes_emitted < 3)) {
            code_string.append("00");
            ++bytes_emitted;
            --bytes_remaining;
        }

        temp.append(fmt::format("\n        {:<6}", code_string));
    }
    return temp;
}

template <typename address_size_t> std::string masm::ir::dot_align<address_size_t>::generate_source_string() const {
    std::string symbol_string;
    if (this->symbol_entry != nullptr) {
        symbol_string = this->symbol_entry->name + ":";
    }
    auto dot_string = ".ALIGN";
    auto operand_string = argument->string();
    return fmt::format("{:<9}{:<8}{:<12}{}", symbol_string, dot_string, operand_string, this->get_formatted_comment());
}

template <typename address_size_t> void masm::ir::dot_align<address_size_t>::set_begin_address(address_size_t addr) {
    auto value = argument->value();
    auto span = (value - (addr % value)) % value;
    this->address_span = {addr, addr + span};
}

template <typename address_size_t> void masm::ir::dot_align<address_size_t>::set_end_address(address_size_t addr) {
    auto value = argument->value();
    auto span = addr % value;
    this->address_span = {addr - span, addr};
}

template <typename address_size_t> address_size_t masm::ir::dot_align<address_size_t>::object_code_bytes() const {
    return this->num_bytes_generated();
}

template <typename address_size_t>
void masm::ir::dot_align<address_size_t>::append_object_code(std::vector<uint8_t> &bytes) const {
    if (!this->emits_object_code)
        return;
    // TODO: Allow fill values other than 0.
    for (auto it = 0; it < this->num_bytes_generated(); it++)
        bytes.emplace_back(0);
}

template <typename address_size_t> address_size_t masm::ir::dot_align<address_size_t>::num_bytes_generated() const {

    return std::get<1>(this->address_span) - std::get<0>(this->address_span);
}

/*
 * .ASCII
 */
template <typename address_size_t> masm::ir::dot_ascii<address_size_t>::dot_ascii() { this->emits_object_code = true; }

template <typename address_size_t>
masm::ir::dot_ascii<address_size_t>::dot_ascii(const masm::ir::dot_ascii<address_size_t> &other) {}

template <typename address_size_t>
masm::ir::dot_ascii<address_size_t> &
masm::ir::dot_ascii<address_size_t>::operator=(masm::ir::dot_ascii<address_size_t> other) {
    swap(*this, other);
    return *this;
}

template <typename address_size_t>
std::shared_ptr<masm::ir::linear_line<address_size_t>> masm::ir::dot_ascii<address_size_t>::clone() const {
    return std::make_shared<dot_ascii<address_size_t>>(*this);
}

template <typename address_size_t> masm::ir::ByteType masm::ir::dot_ascii<address_size_t>::bytes_type() const {
    return masm::ir::ByteType::kData;
}

template <typename address_size_t> std::string masm::ir::dot_ascii<address_size_t>::generate_listing_string() const {
    // Potentially skip codegen
    std::string code_string = "";

    auto bytes_emitted = 0;

    decltype(std::string().substr({}, {})) aliased = this->argument->string();
    auto bytes = masm::byte_vector(aliased);
    auto bytes_head = bytes.begin();
    while (this->emits_object_code && (bytes_head != bytes.end()) && (bytes_emitted < 3)) {
        code_string.append(fmt::format("{:02X}", *bytes_head++));
        ++bytes_emitted;
    }

    auto temp = fmt::format("{:<6} {:<6} {}", fmt::format("{:04X}", this->base_address()), code_string,
                            generate_source_string());

    while (bytes_head != bytes.end()) {
        code_string = "";
        bytes_emitted = 0;
        while (this->emits_object_code && (bytes_head != bytes.end()) && (bytes_emitted < 3)) {
            code_string.append(fmt::format("{:02X}", *bytes_head++));
            ++bytes_emitted;
        }

        temp.append(fmt::format("\n        {:<6}", code_string));
    }
    return temp;
}

template <typename address_size_t> std::string masm::ir::dot_ascii<address_size_t>::generate_source_string() const {
    std::string symbol_string;
    if (this->symbol_entry != nullptr) {
        symbol_string = this->symbol_entry->name + ":";
    }
    auto dot_string = ".ASCII";
    auto operand_string = argument->string();
    return fmt::format("{:<9}{:<8}{:<12}{}", symbol_string, dot_string, "\"" + operand_string + "\"",
                       this->get_formatted_comment());
}

template <typename address_size_t> address_size_t masm::ir::dot_ascii<address_size_t>::object_code_bytes() const {
    auto aliased = this->argument->string();
    return masm::byte_string_length(aliased);
}

template <typename address_size_t>
void masm::ir::dot_ascii<address_size_t>::append_object_code(std::vector<uint8_t> &bytes) const {
    if (!this->emits_object_code)
        return;
    // Must convert to vector using helper to handle escaped codes.
    auto vals = masm::byte_vector(argument->string());
    // Reserve first for improved performance.
    bytes.reserve(bytes.size() + distance(vals.begin(), vals.end()));
    bytes.insert(bytes.end(), vals.begin(), vals.end());
}

/*
 * .BLOCK
 */
template <typename address_size_t> masm::ir::dot_block<address_size_t>::dot_block() { this->emits_object_code = true; }

template <typename address_size_t>
masm::ir::dot_block<address_size_t>::dot_block(const masm::ir::dot_block<address_size_t> &other) {}

template <typename address_size_t>
masm::ir::dot_block<address_size_t> &
masm::ir::dot_block<address_size_t>::operator=(masm::ir::dot_block<address_size_t> other) {
    swap(*this, other);
    return *this;
}

template <typename address_size_t>
std::shared_ptr<masm::ir::linear_line<address_size_t>> masm::ir::dot_block<address_size_t>::clone() const {
    return std::make_shared<dot_block<address_size_t>>(*this);
}

template <typename address_size_t> masm::ir::ByteType masm::ir::dot_block<address_size_t>::bytes_type() const {
    return masm::ir::ByteType::kData;
}

template <typename address_size_t> std::string masm::ir::dot_block<address_size_t>::generate_listing_string() const {
    // Potentially skip codegen
    std::string code_string = "";
    auto bytes_emitted = 0;

    const auto max_bytes = this->argument->value();
    auto fill_value = 0;
    auto bytes_head = 0;
    while (this->emits_object_code && (bytes_head < max_bytes) && (bytes_emitted < 3)) {
        code_string.append(fmt::format("{:02X}", fill_value));
        ++bytes_head;
        ++bytes_emitted;
    }

    auto temp = fmt::vformat("{:<6} {:<6} {}", fmt::make_format_args(fmt::format("{:04X}", this->base_address()),
                                                                     code_string, generate_source_string()));

    while (this->emits_object_code && bytes_head < max_bytes) {
        code_string = "";
        bytes_emitted = 0;
        while (bytes_emitted < 3 && bytes_head < max_bytes) {
            code_string.append(fmt::format("{:02X}", fill_value));
            ++bytes_head;
            ++bytes_emitted;
        }
        std::cout << "here" << std::endl;
        temp.append(fmt::format("\n        {:<6}", code_string));
    }
    return temp;
}

template <typename address_size_t> std::string masm::ir::dot_block<address_size_t>::generate_source_string() const {
    std::string symbol_string;
    if (this->symbol_entry != nullptr) {
        symbol_string = this->symbol_entry->name + ":";
    }
    auto dot_string = ".BLOCK";
    auto operand_string = argument->string();
    return fmt::format("{:<9}{:<8}{:<12}{}", symbol_string, dot_string, operand_string, this->get_formatted_comment());
}

template <typename address_size_t> address_size_t masm::ir::dot_block<address_size_t>::object_code_bytes() const {
    return this->argument->value();
}

template <typename address_size_t>
void masm::ir::dot_block<address_size_t>::append_object_code(std::vector<uint8_t> &bytes) const {
    if (!this->emits_object_code)
        return;
    for (auto it = 0; it < argument->value(); it++)
        bytes.emplace_back(0);
}

template <typename address_size_t> bool masm::ir::dot_block<address_size_t>::tracks_trace_tags() const { return true; }

/*
 * .BURN
 */
template <typename address_size_t>
masm::ir::dot_burn<address_size_t>::dot_burn(const masm::ir::dot_burn<address_size_t> &other) {}

template <typename address_size_t>
masm::ir::dot_burn<address_size_t> &
masm::ir::dot_burn<address_size_t>::operator=(masm::ir::dot_burn<address_size_t> other) {
    swap(*this, other);
    return *this;
}

template <typename address_size_t>
std::shared_ptr<masm::ir::linear_line<address_size_t>> masm::ir::dot_burn<address_size_t>::clone() const {
    return std::make_shared<dot_burn<address_size_t>>(*this);
}

template <typename address_size_t> std::string masm::ir::dot_burn<address_size_t>::generate_listing_string() const {
    auto temp = fmt::format("{:<6} {:<6} {}", "",
                            "", // Doesn't generate any code!
                            generate_source_string());

    return temp;
}

template <typename address_size_t> std::string masm::ir::dot_burn<address_size_t>::generate_source_string() const {

    auto dot_string = ".BURN";
    auto operand_string = argument->string();
    return fmt::format("{:<9}{:<8}{:<12}{}", "", dot_string, operand_string, this->get_formatted_comment());
}

template <typename address_size_t>
void masm::ir::dot_burn<address_size_t>::append_object_code(std::vector<uint8_t> &bytes) const {
    return;
}

/*
 * .BYTE
 */
template <typename address_size_t> masm::ir::dot_byte<address_size_t>::dot_byte() { this->emits_object_code = true; }

template <typename address_size_t>
masm::ir::dot_byte<address_size_t>::dot_byte(const masm::ir::dot_byte<address_size_t> &other) {}

template <typename address_size_t>
masm::ir::dot_byte<address_size_t> &
masm::ir::dot_byte<address_size_t>::operator=(masm::ir::dot_byte<address_size_t> other) {
    swap(*this, other);
    return *this;
}

template <typename address_size_t>
std::shared_ptr<masm::ir::linear_line<address_size_t>> masm::ir::dot_byte<address_size_t>::clone() const {
    return std::make_shared<dot_byte<address_size_t>>(*this);
}

template <typename address_size_t> masm::ir::ByteType masm::ir::dot_byte<address_size_t>::bytes_type() const {
    return masm::ir::ByteType::kData;
}

template <typename address_size_t> std::string masm::ir::dot_byte<address_size_t>::generate_listing_string() const {
    // Potentially skip codegen
    std::string code_string = "";
    if (this->emits_object_code) {
        code_string = fmt::format("{:02X}", this->argument->value() & 0xff);
    }

    auto temp = fmt::format("{:<6} {:<6} {}", fmt::format("{:04X}", this->base_address()), code_string,
                            generate_source_string());

    return temp;
}

template <typename address_size_t> std::string masm::ir::dot_byte<address_size_t>::generate_source_string() const {
    std::string symbol_string;
    if (this->symbol_entry != nullptr) {
        symbol_string = this->symbol_entry->name + ":";
    }
    auto dot_string = ".BYTE";
    auto operand_string = argument->string();
    return fmt::format("{:<9}{:<8}{:<12}{}", symbol_string, dot_string, operand_string, this->get_formatted_comment());
}

template <typename address_size_t> address_size_t masm::ir::dot_byte<address_size_t>::object_code_bytes() const {
    return 1;
}

template <typename address_size_t>
void masm::ir::dot_byte<address_size_t>::append_object_code(std::vector<uint8_t> &bytes) const {
    if (!this->emits_object_code)
        return;
    bytes.emplace_back(argument->value() & 0xff);
    ;
}

template <typename address_size_t> bool masm::ir::dot_byte<address_size_t>::tracks_trace_tags() const { return true; }

/*
 * .END
 */
template <typename address_size_t>
masm::ir::dot_end<address_size_t>::dot_end(const masm::ir::dot_end<address_size_t> &other) {}

template <typename address_size_t>
masm::ir::dot_end<address_size_t> &
masm::ir::dot_end<address_size_t>::operator=(masm::ir::dot_end<address_size_t> other) {
    swap(*this, other);
    return *this;
}

template <typename address_size_t>
std::shared_ptr<masm::ir::linear_line<address_size_t>> masm::ir::dot_end<address_size_t>::clone() const {
    return std::make_shared<dot_end<address_size_t>>(*this);
}

template <typename address_size_t> std::string masm::ir::dot_end<address_size_t>::generate_listing_string() const {
    auto temp = fmt::format("{:<6} {:<6} {}",
                            "", // Doesn't have an address
                            "", // Doesn't generate any code!
                            generate_source_string());

    return temp;
}

template <typename address_size_t> std::string masm::ir::dot_end<address_size_t>::generate_source_string() const {
    auto dot_string = ".END";
    return fmt::format("{:<9}{:<8}{:<12}{}", "", dot_string, "", this->get_formatted_comment());
}

template <typename address_size_t>
void masm::ir::dot_end<address_size_t>::append_object_code(std::vector<uint8_t> &bytes) const {
    return;
}

/*
 * .EQUATE
 */

template <typename address_size_t>
masm::ir::dot_equate<address_size_t>::dot_equate(const masm::ir::dot_equate<address_size_t> &other) {}

template <typename address_size_t>
masm::ir::dot_equate<address_size_t> &
masm::ir::dot_equate<address_size_t>::operator=(masm::ir::dot_equate<address_size_t> other) {
    swap(*this, other);
    return *this;
}

template <typename address_size_t>
std::shared_ptr<masm::ir::linear_line<address_size_t>> masm::ir::dot_equate<address_size_t>::clone() const {
    return std::make_shared<dot_equate<address_size_t>>(*this);
}

template <typename address_size_t> std::string masm::ir::dot_equate<address_size_t>::generate_listing_string() const {
    auto temp = fmt::format("{:<6} {:<6} {}", "", "", generate_source_string());

    return temp;
}

template <typename address_size_t> std::string masm::ir::dot_equate<address_size_t>::generate_source_string() const {
    std::string symbol_string;
    if (this->symbol_entry != nullptr) {
        symbol_string = this->symbol_entry->name + ":";
    }
    auto dot_string = ".EQUATE";
    auto operand_string = argument->string();
    return fmt::format("{:<9}{:<8}{:<12}{}", symbol_string, dot_string, operand_string, this->get_formatted_comment());
}

template <typename address_size_t>
void masm::ir::dot_equate<address_size_t>::append_object_code(std::vector<uint8_t> &bytes) const {
    return;
}

template <typename address_size_t> bool masm::ir::dot_equate<address_size_t>::tracks_trace_tags() const { return true; }

/*
 * .INPUT
 */

template <typename address_size_t>
masm::ir::dot_input<address_size_t>::dot_input(const masm::ir::dot_input<address_size_t> &other) {}

template <typename address_size_t>
masm::ir::dot_input<address_size_t> &
masm::ir::dot_input<address_size_t>::operator=(masm::ir::dot_input<address_size_t> other) {
    swap(*this, other);
    return *this;
}

template <typename address_size_t>
std::shared_ptr<masm::ir::linear_line<address_size_t>> masm::ir::dot_input<address_size_t>::clone() const {
    return std::make_shared<dot_input<address_size_t>>(*this);
}

template <typename address_size_t> std::string masm::ir::dot_input<address_size_t>::generate_listing_string() const {
    auto temp = fmt::format("{:<6} {:<6} {}", "", "", generate_source_string());

    return temp;
}

template <typename address_size_t> std::string masm::ir::dot_input<address_size_t>::generate_source_string() const {
    auto dot_string = ".INPUT";
    auto operand_string = argument->string();
    return fmt::format("{:<9}{:<8}{:<12}{}", "", dot_string, operand_string, this->get_formatted_comment());
}

template <typename address_size_t>
void masm::ir::dot_input<address_size_t>::append_object_code(std::vector<uint8_t> &bytes) const {
    return;
}

template <typename address_size_t> bool masm::ir::dot_input<address_size_t>::tracks_trace_tags() const { return false; }

/*
 * .OUTPUT
 */

template <typename address_size_t>
masm::ir::dot_output<address_size_t>::dot_output(const masm::ir::dot_output<address_size_t> &other) {}

template <typename address_size_t>
masm::ir::dot_output<address_size_t> &
masm::ir::dot_output<address_size_t>::operator=(masm::ir::dot_output<address_size_t> other) {
    swap(*this, other);
    return *this;
}

template <typename address_size_t>
std::shared_ptr<masm::ir::linear_line<address_size_t>> masm::ir::dot_output<address_size_t>::clone() const {
    return std::make_shared<dot_output<address_size_t>>(*this);
}

template <typename address_size_t> std::string masm::ir::dot_output<address_size_t>::generate_listing_string() const {
    auto temp = fmt::format("{:<6} {:<6} {}", "", "", generate_source_string());

    return temp;
}

template <typename address_size_t> std::string masm::ir::dot_output<address_size_t>::generate_source_string() const {
    auto dot_string = ".OUTPUT";
    auto operand_string = argument->string();
    return fmt::format("{:<9}{:<8}{:<12}{}", "", dot_string, operand_string, this->get_formatted_comment());
}

template <typename address_size_t>
void masm::ir::dot_output<address_size_t>::append_object_code(std::vector<uint8_t> &bytes) const {
    return;
}

template <typename address_size_t> bool masm::ir::dot_output<address_size_t>::tracks_trace_tags() const {
    return false;
}

/*
 * .WORD
 */
template <typename address_size_t> masm::ir::dot_word<address_size_t>::dot_word() { this->emits_object_code = true; }

template <typename address_size_t>
masm::ir::dot_word<address_size_t>::dot_word(const masm::ir::dot_word<address_size_t> &other) {}

template <typename address_size_t>
masm::ir::dot_word<address_size_t> &
masm::ir::dot_word<address_size_t>::operator=(masm::ir::dot_word<address_size_t> other) {
    swap(*this, other);
    return *this;
}

template <typename address_size_t>
std::shared_ptr<masm::ir::linear_line<address_size_t>> masm::ir::dot_word<address_size_t>::clone() const {
    return std::make_shared<dot_word<address_size_t>>(*this);
}

template <typename address_size_t> masm::ir::ByteType masm::ir::dot_word<address_size_t>::bytes_type() const {
    return masm::ir::ByteType::kData;
}

template <typename address_size_t> std::string masm::ir::dot_word<address_size_t>::generate_listing_string() const {
    // Potentially skip codegen
    std::string code_string = "";
    if (this->emits_object_code) {
        code_string = fmt::format("{:04X}", this->argument->value());
    }

    auto temp = fmt::format("{:<6} {:<6} {}", fmt::format("{:04X}", this->base_address()), code_string,
                            generate_source_string());

    return temp;
}

template <typename address_size_t> std::string masm::ir::dot_word<address_size_t>::generate_source_string() const {
    std::string symbol_string;
    if (this->symbol_entry != nullptr) {
        symbol_string = this->symbol_entry->name + ":";
    }
    auto dot_string = ".WORD";
    auto operand_string = argument->string();
    return fmt::format("{:<9}{:<8}{:<12}{}", symbol_string, dot_string, operand_string, this->get_formatted_comment());
}

template <typename address_size_t> address_size_t masm::ir::dot_word<address_size_t>::object_code_bytes() const {
    return 2;
}

template <typename address_size_t>
void masm::ir::dot_word<address_size_t>::append_object_code(std::vector<uint8_t> &bytes) const {
    if (!this->emits_object_code)
        return;
    bytes.emplace_back((argument->value() >> 8) & 0xff);
    bytes.emplace_back(argument->value() & 0xff);
}

template <typename address_size_t>
std::optional<std::shared_ptr<const symbol::entry<address_size_t>>>
masm::ir::dot_word<address_size_t>::symbolic_operand() const {
    if (auto as_symbolic = std::dynamic_pointer_cast<masm::ir::symbol_ref_argument<address_size_t>>(argument)) {
        return as_symbolic->symbol_value();
    }
    return std::nullopt;
}

template <typename address_size_t> bool masm::ir::dot_word<address_size_t>::tracks_trace_tags() const { return true; }