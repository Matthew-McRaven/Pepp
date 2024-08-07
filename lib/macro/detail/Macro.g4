grammar Macro;
options { caseInsensitive=true; }

SPACING : [\t ]+ -> skip ; // Spacing is not significant; discard
// We expect to only receive a single line of test into the parser, terminated by a line ending.
// We will ignore the line ending characters in the grammar so that the driver code does not
// need to figure out the correct line terminator for the current file.
NEWLINE : ( [\r] | [\n] | [\r] [\n]) -> skip;

fragment
NameChar
    : NameStartChar
    | '0'..'9'
    | '_'
    ;


fragment
NameStartChar
    : 'a'..'z'
    ;


IDENTIFIER: NameStartChar NameChar* ;
AT_IDENTIFIER: '@' IDENTIFIER ;

fragment DEC_DIGIT: [0-9] ;
UNSIGNED_DECIMAL: DEC_DIGIT+ ;

decl: AT_IDENTIFIER UNSIGNED_DECIMAL ;
