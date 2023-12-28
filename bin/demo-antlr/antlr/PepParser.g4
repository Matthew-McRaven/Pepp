parser grammar PepParser;
options {
    tokenVocab = PepLexer;
}

@parser::members {
    bool allow_macros = 1;
}

// main entry point
prog: chunk* EOF;

// A visitor will need to validate that the mnemonic/IDENTIFIER matches with the argument+type.
instruction
    : (IDENTIFIER|PLACEHOLDER_MACRO) argument (COMMA (IDENTIFIER|PLACEHOLDER_MACRO))? # NonUnaryInstruction
    | (IDENTIFIER|PLACEHOLDER_MACRO) # UnaryInstruction
    ;

// Visitors needed to count number and validate type of arguments.
directive: DOT_IDENTIFIER argument_list?;
invoke_macro: AT_IDENTIFIER argument_list? ;

symbol: SYMBOL | PLACEHOLDER_SYMBOL ;

// A visitor will need to validate that symbol declaration is allowed.
stat
    : symbol? instruction COMMENT? # InstructionLine
    | symbol? directive COMMENT? # DirectiveLine
    | {allow_macros}? symbol? invoke_macro COMMENT? # MacroLine
    | symbol? COMMENT # CommentLine
    ;

// These used to be inlined in the macro_chunk. However, then a line and a macro_chunk had very different "shapes"
// in the AST. These rules allow a macro_chunk to look like a nested "prog", which should make visitors easier.
declare_macro_stat: symbol? DOT_MACRO argument_list COMMENT?;
complete_macro_stat: DOT_ENDM COMMENT? ;
declare_macro_line: declare_macro_stat NEWLINE ;
complete_macro_line: complete_macro_stat (NEWLINE |  {EOF}? );

// A line is either blank, contains one code expression a newline,
// or one code expression and reached the end of the file.
// Don't consume EOF, otherwise prog may allow multiple EOF in one char stream.
// Only match EOF if we are not nested in a macro.
line[bool top_level]: NEWLINE | stat NEWLINE | {$top_level}? stat {EOF}?;
// Absorb all of the lines inside the macro declaration into a single data structure.
// We will need to enter the macro and apply substitutions as necessary.
macro_chunk: declare_macro_line line[false]* complete_macro_line ;
// A chunk is 1+ lines. A normal line only spans one line, but a macro chunk spans multiple.
chunk: {allow_macros}? macro_chunk | line[true] ;

// Perform arity/ type checking in visitors to avoid adding all mnemonics and opcodes here.
// "inline" all possible options instead of using subrules to prevent an ugly, deep parse tree.
argument: HEXADECIMAL | UNSIGNED_DECIMAL | SIGNED_DECIMAL | STRING | CHARACTER | IDENTIFIER | PLACEHOLDER_MACRO;
argument_list: argument (COMMA argument)* ;
