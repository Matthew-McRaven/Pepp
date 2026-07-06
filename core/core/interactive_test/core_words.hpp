#pragma once

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

void native_docol(Interpreter *interp);
void native_exitcol(Interpreter *interp);

void native_halt(Interpreter *interp);