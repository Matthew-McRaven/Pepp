#pragma once

#include "./interp.hpp"
#include "core/integers.h"
class Interpreter;

/*
 * Manipulate elements on stack
 */
// ( a b -- b a)
void native_swap16(Interpreter *interp);
inline static const NativeOpcode Swap16{
    .stack_delta = 0,
    .name = "swap",
    .h = native_swap16,
};

// (i16 -- i16 i16)
void native_dup16(Interpreter *interp);
inline static const NativeOpcode Dup16{
    .stack_delta = 2,
    .name = "dup",
    .h = native_dup16,
};

// (i16 -- )
void native_drop16(Interpreter *interp);
inline static const NativeOpcode Drop16{
    .stack_delta = -2,
    .name = "drop",
    .h = native_drop16,
};

// (a b -- a b a)
void native_over16(Interpreter *interp);
inline static const NativeOpcode Over16{
    .stack_delta = 2,
    .name = "over",
    .h = native_over16,
};

// ( i16 i16 -- i16)
void native_add16i(Interpreter *interp);
inline static const NativeOpcode Add16i{
    .stack_delta = -2,
    .name = "+",
    .h = native_add16i,
};
// Read the byte after this opcode and push it onto the param stack.
// ( -- i16)
void native_lit(Interpreter *interp);
inline static const NativeOpcode Lit{
    .stack_delta = 2,
    .name = "lit",
    .h = native_lit,
};

// Stack initialization
void native_rspinitval(Interpreter *interp);
static const NativeOpcode RspInitVal{
    .stack_delta = 2,
    .name = "r0",
    .h = native_rspinitval,
};
void native_rspstoreval(Interpreter *interp);
static const NativeOpcode RspStoreVal{
    .stack_delta = -2,
    .name = "rsp!",
    .h = native_rspstoreval,
};

/*
 * Words for compilation
 */
void native_docol(Interpreter *interp);
inline static const NativeOpcode Docol{
    .stack_delta = 0,
    .name = "docol",
    .h = native_docol,
};
void native_exitcol(Interpreter *interp);
inline static const NativeOpcode Exitcol{
    .stack_delta = 0,
    .name = "exit",
    .h = native_exitcol,
};
void native_halt(Interpreter *interp);
inline static const NativeOpcode Halt{
    .stack_delta = 0,
    .name = "halt",
    .h = native_halt,
};
void native_latest(Interpreter *interp);
inline static const NativeOpcode Latest{
    .stack_delta = 2,
    .name = "latest",
    .h = native_latest,
};
void native_fetch(Interpreter *interp);
inline static const NativeOpcode Fetch{
    .stack_delta = 0,
    .name = "@",
    .h = native_fetch,
};
void native_hidden(Interpreter *interp);
inline static const NativeOpcode Hidden{
    .stack_delta = -2,
    .name = "hidden",
    .h = native_hidden,
};

/*
 * Words for IO
 */

// (i16 -- )
void native_dot(Interpreter *interp);
inline static const NativeOpcode Dot{
    .stack_delta = -2,
    .name = ".",
    .h = native_dot,
};
// Usually this would buffer IO /inside/ the VM. However, to get INTERP working more quickly, we will delegate all IO to
// the host. ( -- i8 )
void native_key(Interpreter *interp);
inline static const NativeOpcode Key{
    .stack_delta = 2,
    .name = "key",
    .h = native_key,
};
// (i8 -- )
void native_emit(Interpreter *interp);
inline static const NativeOpcode Emit{
    .stack_delta = -2,
    .name = "emit",
    .h = native_emit,
};
// Read individual characters from stdin into a local buffer, searching for the first non-blank character.
// ( -- addr size )
void native_word(Interpreter *interp, u16 buffer_addr);
// ( addr size -- i16)
void native_number(Interpreter *interp);
inline static const NativeOpcode Number{
    .stack_delta = -2,
    .name = "number",
    .h = native_number,
};
void native_lateststore(Interpreter *interp);
inline static const NativeOpcode LatestStore{
    .stack_delta = -2,
    .name = "latest!",
    .h = native_lateststore,
};

/*
 * Dict manip
 */
// ( addr size -- nt|0)
void native_find(Interpreter *interp);
inline static const NativeOpcode Find{
    .stack_delta = -2,
    .name = "find",
    .h = native_find,
};
// (nt -- cfa)
void native_cfa(Interpreter *interp);
inline static const NativeOpcode CFA{
    .stack_delta = 0,
    .name = "cfa",
    .h = native_cfa,
};
// ( addr size -- nt)
void native_create(Interpreter *interp);
inline static const NativeOpcode Create{
    .stack_delta = -2,
    .name = "create",
    .h = native_create,
};
// (i16 -- ), pop data and write to here++
void native_comma(Interpreter *interp);
inline static const NativeOpcode Comma{
    .stack_delta = -2,
    .name = ",",
    .h = native_comma,
};
void native_lbrac(Interpreter *interp);
inline static const NativeOpcode Lbrac{
    .stack_delta = 0,
    .name = "[",
    .h = native_lbrac,
};
void native_rbrac(Interpreter *interp);
inline static const NativeOpcode Rbrac{
    .stack_delta = 0,
    .name = "]",
    .h = native_rbrac,
};

/*
 * Branching
 */
void native_branch(Interpreter *interp);
inline static const NativeOpcode Branch{
    .stack_delta = 0,
    .name = "branch",
    .h = native_branch,
};
void native_zbranch(Interpreter *interp);

/*
 * The interpreter!!
 */
void native_interpret(Interpreter *interp, u16 word_buffer, u16 pcode_lit);

/*
 * Debug tools
 */
void native_dumpdict(Interpreter *interp);
inline static const NativeOpcode DumpDict{
    .stack_delta = 0,
    .name = "dumpdict",
    .h = native_dumpdict,
};

void native_toggle_debug(Interpreter *interp);
inline static const NativeOpcode ToggleDebug{
    .stack_delta = 0,
    .name = "~debug",
    .h = native_toggle_debug,
};

void register_core_words(Interpreter *interp);
