#pragma once

#include "../interp.hpp"
#include "core/integers.h"
class Interpreter;

/*
 * Implemented in core.cpp
 * Register the following words:
 * HALT     ( -- ), stop executing instructions
 * DOCOL    ( -- ), push the next IP onto return stack and set IP to the address of the word being executed
 * EXIT     ( -- ), pop the top of the return stack into IP,
 * WORD     ( -- addr len ), read the next word from input and return its address and length
 * &WORD    ( -- addr ), push the address of the buffer used by WORD onto the param stack
 * CREATE   ( addr len -- nt ), create a new word in the dictionary from a name. Push the NT of the newly created word.
 * HIDDEN   ( nt -- ), mark the word at NT as hidden, so it will not be found by name
 * LATEST   ( -- nt ), push the NT of the latest word onto the param stack
 * LIT      ( -- i16 ), read the 16-bit value at current IP, push it onto param stack, and increment IP.
 * @        ( addr -- i16 ), compute *addr
 * ,        ( i16 -- ), append the 16-bit value on top of the param stack to *here++
 * [        ( -- ), enter IMMEDIATE mode, so that words will be executed instead of compiled
 * ]        ( -- ), enter COMPILE mode, so that words will be compiled instead of executed
 * COREINT  ( -- ), the basic FORTH interpreter implemented in machine code
 * :        ( -- ), read the next word & create a new def. Enter COMPILE mode
 * ;        ( -- ), complete the word started by : and exit COMPILE mode
 * R0       ( -- u16), push the initial value of the return stack pointer onto the param stack
 * RSP!     ( u16-- ), write the value on top of the param stack to the return stack pointer
 * BRANCH   ( -- ), read the next 16-bit value at current IP and add it to IP
 * QUIT     ( -- ), start the interpreter loop, discarding values on the return stack
 */
void register_core_words(Interpreter *interp);
// Helper method if all your opcode does is push a constant onto PSP.
void push_constant(Interpreter *interp, u16 addr);
// Given a pointer to a temp buffer, the implementation of the WORD operation.
// Read individual characters from stdin into a local buffer, searching for the first non-blank character.
u16 word_helper(Interpreter *interp, u16 buffer_addr);
// Shared number parsing logic "hardware" routine. (addr, size) are a pointer, probably from WORD.
std::optional<i32> number_helper(Interpreter *interp, u16 addr, u16 size);
// Shared "harware" routine for traversing the dictionary to find a word from its name.
u16 find_helper(Interpreter *interp, u16 addr, u16 size);

/*
 * Implemented in arithmetic.cpp
 * Register the following words:
 * +     ( i16 i16 -- i16 ), add the top two words of the stack
 * -     ( i16 i16 -- i16 ), subtract the top two words of the stack
 */
void register_arithmetic_words(Interpreter *interp);

/*
 * Implemented in stack.cpp
 * Register the following words:
 * SWAP  ( a b -- b a ), swap the top two words of the stack
 * DUP   ( i16 -- i16 i16 ), duplicate the top word of the stack
 * DROP  ( i16 -- ), drop the top word of the stack
 * OVER  ( a b -- a b a ), copy the second word of the stack to the top
 */
void register_stack_words(Interpreter *interp);

/*
 * Implemented in memory.cpp
 * Register the following words:
 * CMOVE  ( src cnt dst -- ), copy cnt bytes from src to dst
 * CMOVE0 ( src cnt dst -- ), copy cnt bytes from src to dst and append a null byte to dst
 *
 * @ and ! are already defined in core_words as a requirement of :;
 */
void register_memory_words(Interpreter *interp);

/*
 * Implemented in io.cpp
 * Register the following words:
 * .     ( i16 -- ), print the top word of the stack as a signed integer
 * KEY   ( -- i8 ), read a character from input and push it onto the stack
 * EMIT  ( i8 -- ), write the character on top of the stack to output
 * NUMBER( addr len -- i16), convert the string at addr with length len to an integer and push it onto the stack
 * PRINT0( addr -- ), print the null-terminated string at addr to output
 *
 * WORD and &WORD are implemented in core as a requirement of find.
 * Helpers for WORD and NUMBER are also implemented in core.
 */
void register_io_words(Interpreter *interp);
void native_key(Interpreter *interp);

/*
 * Implemented in debug.cpp
 * Register the following words:
 * dumpdict ( -- ), print the contents of the dictionary to output
 * ~debug   ( -- ), toggle debug mode on or off
 */
void register_debug_words(Interpreter *interp);

/*
 * Implemented in sys_globals.cpp
 * Register the following words:
 * PSP     ( -- u16), push the current value of the param stack pointer onto the param stack
 * RSP     ( -- u16), push the current value of the return stack pointer onto the param stack
 * HERE    ( -- u16), push the current value of the HERE pointer onto the param stack
 * LATEST! ( u16 -- ), write the value on top of the param stack to the LATEST pointer
 */
void register_sys_globals_words(Interpreter *interp);

/*
 * Implemented in dict.cpp
 * Register the following words:
 * FIND    ( addr len -- nt|0), find the word with the given name and push its NT onto the stack, or 0 if not found
 * CFA     ( nt -- cfa), push the code field address of the word at NT onto the stack
 */
void register_dict_words(Interpreter *interp);

/*
 * Implemented in control.cpp
 * Register the following words:
 * BRANCH0 ( i16 -- ), pop the top of the stack, if it is 0, read the next 16-bit value at current IP and add it to IP
 */
void register_control_words(Interpreter *interp);

// Register all of the above words into the interpreter.
// This is a good starting point for a general-purpose VM.
void register_common_words(Interpreter *interp);