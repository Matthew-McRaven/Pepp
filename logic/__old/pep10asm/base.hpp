#pragma once

#include <memory>
#include <variant>
#include <vector>
#include "symbol/entry.hpp"

namespace masm::ir {

enum class ByteType
{
    kData, // The line injects bytes into the bytecode, but is not meant to be executed.
    kCode, // The line injects bytes into the bytecode, and is meant to be executed.
    kNoBytes // The line does not inject bytes into the bytecode.
};
// Represent a single line of a linear ir code
template <typename address_size_t>
class linear_line
{
public:
	using symbol_t = symbol::entry<address_size_t>;
    linear_line();
    linear_line(const linear_line& other);
    virtual ~linear_line() = 0;
    // Cannot support operator= in AsmCode, it is pure virtual.
    virtual std::shared_ptr<linear_line> clone() const = 0;

    // Can this line have trace tags?
    virtual bool tracks_trace_tags() const {return false;}
    // If this line generates bytes in the byte stream, are those bytes code or data?
    virtual ByteType bytes_type() const {return ByteType::kNoBytes;}

    // Detailed information about how the instruction interacts with the memory trace.
    //QList<TraceCommand> getTraceData() const;
    //void setTraceData(QList<TraceCommand> trace);

    // Can the code line be addressed?
    virtual bool contains_memory_address() {return object_code_bytes() != 0; } 
    virtual address_size_t base_address() const {return std::get<0>(address_span);};
    virtual void set_begin_address(address_size_t addr) {address_span = {addr, addr+object_code_bytes()-1};}
    virtual void set_end_address(address_size_t addr) {address_span = {addr-object_code_bytes()+1, addr};}
    
    // Get the assembler listing, which is memaddress + object code + sourceLine.
    virtual std::string generate_listing_string() const = 0;
    // Returns the properly formatted source line.
    virtual std::string generate_source_string() const = 0;
    virtual address_size_t object_code_bytes() const {return 0;}

    virtual bool is_code() const { return false;}
    virtual void append_object_code(std::vector<uint8_t>& code) const = 0;
    std::string get_formatted_comment() const;


    virtual std::optional<std::shared_ptr<const symbol_t>> symbolic_operand() const { return nullptr;}
    friend void swap(linear_line& first, linear_line& second)
    {
        using std::swap;
        swap(first.emits_object_code, second.emits_object_code);
        swap(first.address_span, second.address_span);
        swap(first.breakpoint, second.breakpoint);
        swap(first.comment, second.comment);
        swap(first.source_line, second.source_line);
        swap(first.listing_line, second.listing_line);
        swap(first.symbol_entry, second.symbol_entry);
        //swap(first.trace, second.trace);

    }

    // Set if object code should be generated for this code line. If an instruction is before
    // a .BURN directive, then this should be set to false. If this is false, object code
    // length should be 0.
    bool emits_object_code = {false};
    // Not all lines are capable of having a breakpoint (comments), so default to empty rather than false.
    std::optional<bool> breakpoint = {};
    // Track if the current line has a comment, and if so, what is it.
    std::optional<std::string> comment = {};
    // The line number (0 indexed) of the line of code in the source program or listing..
    std::size_t source_line = 0, listing_line = 0;

    // Before attempting to use, check if the symbol is null.
    // Dereferencing an empty shared pointer causes memory access violatations that are hard to debug.
    std::shared_ptr<symbol_t> symbol_entry = nullptr;
    
    // Information collected during assembly to enable memory tracing features.
    // QList<TraceCommand> trace;
protected:
    // What memory region is spanned by this instruction.
    std::tuple<address_size_t, address_size_t> address_span;
};
}; // End namespace masm::ir.
#include "base.tpp"