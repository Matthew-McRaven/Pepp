lexer grammar PeppLexer;
options { caseInsensitive=true; }

SPACING : [\t ]+ -> skip ; // Spacing is not significant; discard
NEWLINE : [\r]?[\n]; // Newlines are significant to formatting; keep

// Greedily match not \ or quote
// If matching \, immediately consume the next character
STRING : '"' (~[\\"] | '\\' ~[\r\n] | '\\x' HEX_DIGIT HEX_DIGIT)* '"';
// Ibid with apostrophe
CHARACTER: '\u0027' (~[\\\u0027] | '\\' . | '\\x' HEX_DIGIT HEX_DIGIT) '\u0027';

fragment
NameChar
    : NameStartChar
    | '0'..'9'
    /*| '\u00B7'
    | '\u0300'..'\u036F'
    | '\u203F'..'\u2040'*/
    ;


fragment
NameStartChar
    : 'a'..'z'
    | '_'
    /*| '\u00C0'..'\u00D6'
    | '\u00D8'..'\u00F6'
    | '\u00F8'..'\u02FF'
    | '\u0370'..'\u037D'
    | '\u037F'..'\u1FFF'
    | '\u200C'..'\u200D'
    | '\u2070'..'\u218F'
    | '\u2C00'..'\u2FEF'
    | '\u3001'..'\uD7FF'
    | '\uF900'..'\uFDCF'
    | '\uFDF0'..'\uFFFD'*/
    ;


IDENTIFIER: NameStartChar NameChar* ;

DOLLAR: '$';
PLACEHOLDER_MACRO: DOLLAR DEC_DIGIT+ ;

// Helper to make matching directives and macro invocations easier.
DOT_IDENTIFIER: '.' IDENTIFIER ;
AT_IDENTIFIER: '@' IDENTIFIER ;

COLON: ':' ;
SYMBOL: IDENTIFIER COLON;
PLACEHOLDER_SYMBOL: PLACEHOLDER_MACRO COLON;

fragment DEC_DIGIT: [0-9] ;
fragment HEX_DIGIT: DEC_DIGIT | [a-f];
UNSIGNED_DECIMAL: DEC_DIGIT+ ;
SIGNED_DECIMAL: ('+'|'-') DEC_DIGIT+ ;
HEXADECIMAL: '0x' HEX_DIGIT+ ;

fragment SEMICOLON: ';';
COMMENT: SEMICOLON COMMENT_BODY;
fragment COMMENT_BODY: ~[\r\n]*;

COMMA: ',' ;
