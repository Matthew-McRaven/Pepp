#pragma once

#include <memory>
#include <optional>

#include "base.hpp"
#include "args.hpp"
namespace masm::ir {

    
template <typename address_size_t>
class dot_address: public masm::ir::linear_line<address_size_t>
{
public:

    dot_address() = default;
    ~dot_address() override = default;
    dot_address(const dot_address& other);
    dot_address& operator=(dot_address other);
    std::shared_ptr<linear_line<address_size_t> > clone() const override;

    // linear_line interface
    std::string generate_listing_string() const override;
    std::string generate_source_string() const override;
    address_size_t object_code_bytes() const override;
    void append_object_code(std::vector<uint8_t>& code) const override;

    std::optional<std::shared_ptr<const symbol::entry<address_size_t>>> symbolic_operand() const override;


    friend void swap(dot_address& first, dot_address& second)
    {
        using std::swap;
        swap(static_cast<linear_line<address_size_t>&>(first), static_cast<linear_line<address_size_t>&>(second));
        swap(first.argument, second.argument);
    }
    std::shared_ptr<masm::ir::symbol_ref_argument<address_size_t> >argument = {nullptr};
};



template <typename address_size_t>
class dot_align: public masm::ir::linear_line<address_size_t>
{
public:
    enum class align_direction
    {
        kNext, // The next byte should be aligned properly
        kPrevious // The previous byte should be aligned properly
    };
    dot_align();
    virtual ~dot_align() override = default;
    dot_align(const dot_align& other);
    dot_align& operator=(dot_align other);
    std::shared_ptr<linear_line<address_size_t> > clone() const override;;

    // linear_line interface
    std::string generate_listing_string() const override;
    std::string generate_source_string() const override;
    address_size_t object_code_bytes() const override;
    void append_object_code(std::vector<uint8_t>& code) const override;

    void set_begin_address(address_size_t addr) override;
    void set_end_address(address_size_t addr) override;
    address_size_t num_bytes_generated() const;


    friend void swap(dot_align& first, dot_align& second)
    {
        using std::swap;
        swap(static_cast<linear_line<address_size_t>&>(first), static_cast<linear_line<address_size_t>&>(second));
        swap(first.argument, second.argument);
        swap(first.direction, second.direction);
    }

    std::shared_ptr<masm::ir::lir_argument<address_size_t>> argument = {nullptr};
    align_direction direction = {align_direction::kNext};
};

template <typename address_size_t>
class dot_ascii: public masm::ir::linear_line<address_size_t>
{
public:
    dot_ascii();
    ~dot_ascii() override = default;
    dot_ascii(const dot_ascii& other);
    dot_ascii& operator=(dot_ascii other);
    std::shared_ptr<linear_line<address_size_t> > clone() const override;

    // linear_line interface
    std::string generate_listing_string() const override;
    std::string generate_source_string() const override;
    address_size_t object_code_bytes() const override;
    void append_object_code(std::vector<uint8_t>& code) const override;

    friend void swap(dot_ascii& first, dot_ascii& second)
    {
        using std::swap;
        swap(static_cast<linear_line<address_size_t>&>(first), static_cast<linear_line<address_size_t>&>(second));
        swap(first.argument, second.argument);
    }

    std::shared_ptr<masm::ir::ascii_argument<address_size_t>> argument = {nullptr};
};


template <typename address_size_t>
class dot_block: public masm::ir::linear_line<address_size_t>
{
public:
    dot_block();
    ~dot_block() override = default;
    dot_block(const dot_block& other);
    dot_block& operator=(dot_block other);
    std::shared_ptr<linear_line<address_size_t> > clone() const override;

    // linear_line interface
    std::string generate_listing_string() const override;
    std::string generate_source_string() const override;
    address_size_t object_code_bytes() const override;
    void append_object_code(std::vector<uint8_t>& code) const override;

    bool tracks_trace_tags() const override;

    friend void swap(dot_block& first, dot_block& second)
    {
        using std::swap;
        swap(static_cast<linear_line<address_size_t>&>(first), static_cast<linear_line<address_size_t>&>(second));;
        swap(first.argument, second.argument);
    }

    std::shared_ptr<masm::ir::lir_argument<address_size_t> > argument = nullptr;
};


template <typename address_size_t>
class dot_burn: public masm::ir::linear_line<address_size_t>{
public:
    dot_burn() = default;
    virtual ~dot_burn() override = default;
    dot_burn(const dot_burn& other);
    dot_burn& operator=(dot_burn other);
    std::shared_ptr<linear_line<address_size_t> > clone() const override;;
    void append_object_code(std::vector<uint8_t>& code) const override;

    // linear_line interface
    std::string generate_listing_string() const override;
    std::string generate_source_string() const override;


    friend void swap(dot_burn& first, dot_burn& second)
    {
        using std::swap;
        swap(static_cast<linear_line<address_size_t>&>(first), static_cast<linear_line<address_size_t>&>(second));;
        swap(first.argument, second.argument);
    }
    std::shared_ptr<masm::ir::lir_argument<address_size_t> > argument = nullptr;;
};

template <typename address_size_t>
class dot_byte: public masm::ir::linear_line<address_size_t>
{
public:
    dot_byte();
    ~dot_byte() override = default;
    dot_byte(const dot_byte& other);
    dot_byte& operator=(dot_byte other);
    std::shared_ptr<linear_line<address_size_t> > clone() const override;

    // linear_line interface
    std::string generate_listing_string() const override;
    std::string generate_source_string() const override;
    address_size_t object_code_bytes() const override;
    void append_object_code(std::vector<uint8_t>& code) const override;

    bool tracks_trace_tags() const override;

    friend void swap(dot_byte& first, dot_byte& second)
    {
        using std::swap;
        swap(static_cast<linear_line<address_size_t>&>(first), static_cast<linear_line<address_size_t>&>(second));;
        swap(first.argument, second.argument);
    }

    std::shared_ptr<masm::ir::lir_argument<address_size_t> > argument = nullptr;
};


template <typename address_size_t>
class dot_end: public masm::ir::linear_line<address_size_t>
{
public:
    dot_end() = default;
    ~dot_end() override = default;
    dot_end(const dot_end& other);
    dot_end& operator=(dot_end other);
    std::shared_ptr<linear_line<address_size_t> > clone() const override;

    // linear_line interface
    std::string generate_listing_string() const override;
    std::string generate_source_string() const override;
    void append_object_code(std::vector<uint8_t>& code) const override;

    friend void swap(dot_end& first, dot_end& second)
    {
        using std::swap;
        swap(static_cast<linear_line<address_size_t>&>(first), static_cast<linear_line<address_size_t>&>(second));
    }
};


template <typename address_size_t>
class dot_equate: public masm::ir::linear_line<address_size_t>
{
public:
    dot_equate() = default;
    ~dot_equate() override = default;
    dot_equate(const dot_equate& other);
    dot_equate& operator=(dot_equate other);
    std::shared_ptr<linear_line<address_size_t> > clone() const override;

    // linear_line interface
    std::string generate_listing_string() const override;
    std::string generate_source_string() const override;
    void append_object_code(std::vector<uint8_t>& code) const override;

    bool tracks_trace_tags() const override;

    friend void swap(dot_equate& first, dot_equate& second)
    {
        using std::swap;
        swap(static_cast<linear_line<address_size_t>&>(first), static_cast<linear_line<address_size_t>&>(second));
        swap(first.argument, second.argument);
    }

    std::shared_ptr<masm::ir::lir_argument<address_size_t> > argument = nullptr;
};

template <typename address_size_t>
class dot_input: public masm::ir::linear_line<address_size_t>
{
public:
    dot_input() = default;
    ~dot_input() override = default;
    dot_input(const dot_input& other);
    dot_input& operator=(dot_input other);
    std::shared_ptr<linear_line<address_size_t> > clone() const override;

    // linear_line interface
    std::string generate_listing_string() const override;
    std::string generate_source_string() const override;
    void append_object_code(std::vector<uint8_t>& code) const override;

    bool tracks_trace_tags() const override;

    friend void swap(dot_input& first, dot_input& second)
    {
        using std::swap;
        swap(static_cast<linear_line<address_size_t>&>(first), static_cast<linear_line<address_size_t>&>(second));
        swap(first.argument, second.argument);
    }

    std::shared_ptr<masm::ir::symbol_ref_argument<address_size_t> > argument = nullptr;
};

template <typename address_size_t>
class dot_output: public masm::ir::linear_line<address_size_t>
{
public:
    dot_output() = default;
    ~dot_output() override = default;
    dot_output(const dot_output & other);
    dot_output& operator=(dot_output other);
    std::shared_ptr<linear_line<address_size_t> > clone() const override;

    // linear_line interface
    std::string generate_listing_string() const override;
    std::string generate_source_string() const override;
    void append_object_code(std::vector<uint8_t>& code) const override;

    bool tracks_trace_tags() const override;

    friend void swap(dot_output& first, dot_output& second)
    {
        using std::swap;
        swap(static_cast<linear_line<address_size_t>&>(first), static_cast<linear_line<address_size_t>&>(second));
        swap(first.argument, second.argument);
    }

    std::shared_ptr<masm::ir::symbol_ref_argument<address_size_t> > argument = nullptr;
};

template <typename address_size_t>
class dot_word: public masm::ir::linear_line<address_size_t>
{
public:
    dot_word();
    ~dot_word() override = default;
    dot_word(const dot_word& other);
    dot_word& operator=(dot_word other);
    std::shared_ptr<linear_line<address_size_t> > clone() const override;

    // linear_line interface
    std::string generate_listing_string() const override;
    std::string generate_source_string() const override;
    address_size_t object_code_bytes() const override;
    void append_object_code(std::vector<uint8_t>& code) const override;

    std::optional<std::shared_ptr<const symbol::entry<address_size_t>>> symbolic_operand() const override;

    bool tracks_trace_tags() const override;

    friend void swap(dot_word& first, dot_word& second)
    {
        using std::swap;
        swap(static_cast<linear_line<address_size_t>&>(first), static_cast<linear_line<address_size_t>&>(second));;
        swap(first.argument, second.argument);
    }

    std::shared_ptr<masm::ir::lir_argument<address_size_t> > argument = nullptr;
};

};
#include "directives.tpp"
