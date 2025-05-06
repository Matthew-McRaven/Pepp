parser grammar PeppParser;
// Execute: antlr4 -Dlanguage=Cpp -visitor PeppLexer.g4 PeppParser.g4 -package parse
options {
    tokenVocab = PeppLexer;
}

@parser::members {
bool allow_macro_invocations = 1;
// If true, the AST is expected to perform macro substitutions, which may require re-parsing some lines.
// If false, macros require a pre-processor to perform macro substitution.
bool allow_deferred_macros = 0;
}


prog: (NEWLINE | stat NEWLINE | stat {EOF}?)* EOF;

// A visitor will need to validate that the mnemonic/IDENTIFIER matches with the argument+type.
instruction
    : IDENTIFIER argument (COMMA (IDENTIFIER|PLACEHOLDER_MACRO))? # NonUnaryInstruction
    | IDENTIFIER # UnaryInstruction
    ;

// Visitors needed to count number and validate type of arguments.
directive: DOT_IDENTIFIER argument_list?;
invoke_macro: AT_IDENTIFIER argument_list? ;

symbol: SYMBOL | {allow_deferred_macros}? PLACEHOLDER_SYMBOL ;

// A visitor will need to validate that symbol declaration is allowed.
stat
    : symbol? instruction COMMENT? # InstructionLine
    | symbol? directive COMMENT? # DirectiveLine
    | {allow_macro_invocations}? symbol? invoke_macro COMMENT? # MacroInvokeLine
    | symbol? COMMENT # CommentLine
    // Until the PLACEHOLDER_MACRO has been resolved to text, we can't know which kind of stat it is.
    | {allow_deferred_macros}? symbol? PLACEHOLDER_MACRO argument_list? COMMENT? # DeferredLine
    ;

// Perform arity/ type checking in visitors to avoid adding all mnemonics and opcodes here.
// "inline" all possible options instead of using subrules to prevent an ugly, deep parse tree.
argument: HEXADECIMAL | UNSIGNED_DECIMAL | SIGNED_DECIMAL | STRING | CHARACTER | IDENTIFIER | {allow_deferred_macros}? PLACEHOLDER_MACRO;
argument_list: argument (COMMA argument)* ;
