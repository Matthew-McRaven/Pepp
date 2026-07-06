#pragma once

#include "core/integers.h"
class Interpreter;

/*
 * Manipulate elements on stack
 */
// ( a b -- b a)
void native_swap16(Interpreter *interp);
// (i16 -- i16 i16)
void native_dup16(Interpreter *interp);
// (i16 -- )
void native_drop16(Interpreter *interp);
// (a b -- a b a)
void native_over16(Interpreter *interp);

// ( i16 i16 -- i16)
void native_add16i(Interpreter *interp);
void native_lit(Interpreter *interp);

/*
 * Words for compilation
 */
void native_docol(Interpreter *interp);
void native_exitcol(Interpreter *interp);
void native_halt(Interpreter *interp);
void native_latest(Interpreter *interp);
void native_fetch(Interpreter *interp);
void native_hidden(Interpreter *interp);

/*
 * Words for IO
 */

// Usually this would buffer IO /inside/ the VM. However, to get INTERP working more quickly, we will delegate all IO to
// the host. ( -- i8 )
void native_key(Interpreter *interp);
// (i8 -- )
void native_emit(Interpreter *interp);
// Read individual characters from stdin into a local buffer, searching for the first non-blank character.
// ( -- addr size )
void native_word(Interpreter *interp, u16 buffer_addr);
// ( addr size -- i16)
void native_number(Interpreter *interp);
void native_lateststore(Interpreter *interp);

/*
 * Dict manip
 */
// ( addr size -- nt|0)
void native_find(Interpreter *interp);
// (nt -- cfa)
void native_cfa(Interpreter *interp);
// ( addr size -- nt)
void native_create(Interpreter *interp);
// (i16 -- ), pop data and write to here++
void native_comma(Interpreter *interp);
void native_lbrac(Interpreter *interp);
void native_rbrac(Interpreter *interp);

/*
 * Branching
 */
void native_branch(Interpreter *interp);
void native_zbranch(Interpreter *interp);

/*
 * The interpreter!!
 */
void native_interpret(Interpreter *interp, u16 word_buffer, u16 pcode_lit);
