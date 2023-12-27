grammar Expr;		
prog:	expr EOF ;
expr:	expr ('*'|'/') expr
    |	expr ('+'|'-') expr
    |	INT
    ;
SPACING : [ \t ]+ -> channel(HIDDEN)
NEWLINE : [\r\n]+ -> skip;
INT     : [0-9]+ ;
