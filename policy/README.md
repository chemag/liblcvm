# Notes on policy definition


Goal: Easy definition of syntax for warnings or errors.

We want to implement a DSL that allows users to define warning/error messages based on CSV column values. The idea is that the user defines warning/errors as follows:

```
warn "broken col" (column1 == 5 or column2 != 8) and not (column3 in range(1, 10))
error "1" (column1 == 6 or column2 == 8) and not (column1 in range(2, 10))
error "2" (column1 == 5 or column2 != 8) and not (column2 in range(1, 10))
```

The output of a series of rules will be either a warn and an error string. The strings will contain the name of the rul (the string after "warn/error") and the values used to match the rule (in the first row, column1, column2, and column3).

Note that the operations we want to support are "==", "!=", ">", ">=", "<", "<=". The first 2 will work in both numbers and strings, the last 4 only on numbers. We also want to support the syntax "column in range(const1, const2)", and logical operations (and, or, not, and parentheses for priorities).

This file will be converted into a protobuf message, so we can run it safely in cpp.

We also include a C++ evaluator example.

This is user-defined file with several of these lines to a protobuf implementation of the grammar (so we can run it in cpp).


# Architecture

2-layer architecture:
* (1) user-facing DSL: We define a concise, human-friendly syntax that makes users happy at writing checks. Syntax is parsed into an AST (Abstract Syntax Tree). The AST is compiled into a protobuf representation. Actually, instead of the AST, we use an existing parser generator (PEGTL or ANTLR)
* (2) protobuf-based expression tree: We have a protobuf-based structure with the same syntax of the user-facing DSL. This provides simple evaluation (C++) and storage/transmission.

Advantages of the approach:
* human-writable syntax (friendly, intuitive, easy to read/write)
* structured internal representation (safe, portable, fast in C++)


Example: User-syntax
```
warn "warn 1" (age == 30 and name != "Bob") or not (score in range(1, 2))
```

This would serialize into the following protobuf:
```
rules {
  label: "warn 1"
  condition {
    logical {
      op: OR
      operands {
        logical {
          operands {
            comparison {
              column: "age"
              op: EQ
              value: "30"
            }
          }
          operands {
            comparison {
              column: "name"
              op: NE
              value: "Bob"
            }
          }
        }
      }
      operands {
        not_expr {
          expr {
            range {
              column: "score"
              low: 1
              high: 2
            }
          }
        }
      }
    }
  }
}
```


# Syntax

Support:
* Comparison: ==, !=, <, <=, >, >=
* Ranges: in range(a, b) or custom between
* Logical: AND, OR, NOT
* Parentheses for grouping

Grammar:

```
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
```

# TODO

* integrate into liblcvm
* add unittest
* add fuzzing
