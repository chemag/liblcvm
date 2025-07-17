grammar rules;

// === Parser Rules ===

program     : statement+ EOF ;

statement   : (WARN | ERROR) STRING expr ;

expr
    : expr OR expr                         # OrExpr
    | expr AND expr                        # AndExpr
    | NOT expr                             # NotExpr
    | LPAREN expr RPAREN                   # ParenExpr
    | IDENT compOp value                   # CompareExpr
    | IDENT IN RANGE LPAREN number COMMA number RPAREN  # RangeExpr
    ;

compOp
    : EQ
    | NE
    | LT
    | GT
    | LE
    | GE
    ;

value
    : STRING
    | number
    ;

number
    : NUMBER
    ;

// === Lexer Rules ===

OR      : 'or';
AND     : 'and';
NOT     : 'not';
IN      : 'in';
RANGE   : 'range';

EQ      : '==';
NE     : '!=';
LT      : '<';
GT      : '>';
LE      : '<=';
GE      : '>=';

LPAREN  : '(';
RPAREN  : ')';
COMMA   : ',';

WARN    : 'warn';
ERROR   : 'error';

IDENT   : [a-zA-Z_][a-zA-Z_0-9]*;
NUMBER  : [0-9]+ ('.' [0-9]+)?;
STRING  : '"' (~["\\] | '\\' .)* '"';
WS      : [ \t\r\n]+ -> skip;

