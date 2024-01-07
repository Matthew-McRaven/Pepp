parser grammar PeppParser;
options {
    tokenVocab = PepLexer;
}

@parser::members {
bool allow_macros = 1;
}


prog: (NEWLINE | stat NEWLINE | stat {EOF}?)* EOF;

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

// Perform arity/ type checking in visitors to avoid adding all mnemonics and opcodes here.
// "inline" all possible options instead of using subrules to prevent an ugly, deep parse tree.
argument: HEXADECIMAL | UNSIGNED_DECIMAL | SIGNED_DECIMAL | STRING | CHARACTER | IDENTIFIER | PLACEHOLDER_MACRO;
argument_list: argument (COMMA argument)* ;
