#pragma once

#include <memory>
#include <variant>
#include "symbol/entry.hpp"

namespace masm::ir {

// Represent a single line of a linear ir code
template <typename address_size_t>
class linear_line
{
public:
	using symbol_t = symbol::SymbolEntry<address_size_t>;
    linear_line();
    linear_line(const linear_line& other);
    virtual ~linear_line() = 0;
    // Cannot support operator= in AsmCode, it is pure virtual.
    virtual std::shared_ptr<linear_line> clone() const = 0;

    // Can this line have trace tags?
    virtual bool tracks_trace_tags() const {return false;}

    // Detailed information about how the instruction interacts with the memory trace.
    //QList<TraceCommand> getTraceData() const;
    //void setTraceData(QList<TraceCommand> trace);

    // Can the code line be addressed?
    virtual bool contains_memory_address() {return false; } 


    // Get the assembler listing, which is memaddress + object code + sourceLine.
    virtual std::string generate_listing_string() const = 0;
    // Returns the properly formatted source line.
    virtual std::string generate_source_string() const = 0;
    virtual address_size_t object_code_bytes() const {return 0;}

    virtual bool is_code() const { return false;}


    virtual bool has_symbolic_operand() const {return false;}
    virtual std::shared_ptr<symbol_t> get_symbolic_operand() const { return nullptr;}
    friend void swap(linear_line& first, linear_line& second)
    {
        using std::swap;
        swap(first.emits_object_code, second.emits_object_code);
        swap(first.base_address, second.base_address);
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
    // What is the address of this line of code, if it emits object code?
    address_size_t base_address = {0};
    //Not all lines are capable of having a breakpoint (comments), so default to empty rather than false.
    std::variant<std::monostate, bool> breakpoint = {std::monostate()};
    // Track if the current line has a comment, and if so, what is it.
    std::variant<std::monostate, std::string> comment = {std::monostate()};
    // The line number (0 indexed) of the line of code in the source program or listing..
    address_size_t source_line = 0, listing_line = 0;

    // Before attempting to use, check if the symbol is null.
    // Dereferencing an empty shared pointer causes memory access violatations that are hard to debug.
    std::shared_ptr<symbol_t> symbol_entry = nullptr;
    
    // Information collected during assembly to enable memory tracing features.
    // QList<TraceCommand> trace;
};
}; // End namespace masm::ir.
#include "base.tpp"