# Syntax Grammar

```ebnf
program       -> declaration* EOF ;
```

## Declarations

```ebnf
declaration   -> classDecl
               | funcDecl
               | varDecl
               | finalDecl
               | statement ;

classDecl     -> "class" IDENTIFIER ":" function* "end" ;
funcDecl      -> "func" function ;
varDecl       -> TYPE IDENTIFIER ( "=" expression )? ";" ;
finalDecl     -> "final" TYPE IDENTIFIER "=" expression ";" ;
```

## Statements

```ebnf
statement     -> exprStmt
               | forStmt
               | ifStmt
               | printStmt
               | returnStmt
               | whileStmt 
               | block ; 

exprStmt      -> expression ";" ;
forStmt       -> "for" "(" ( varDecl | exprStmt | ";" )
                           expression? ";"
                           expression? ")"
                           ":" statement "end" ;
ifStmt        -> "if" expression ":" 
                 statement
                 ( "else" ("if" expression ) ":" statement  )* 
                 "end" ;
printStmt     -> "print" "(" expression ")" ";" ;
returnStmt    -> "return" expression? ";" ;
whileStmt     -> "while" "(" expression ")" statement ;
block         -> declaration* ;
```

## Expressions

```ebnf
expression    -> assignment ;

assignment    -> ( call "." )? IDENTIFIER "=" assignment
               | logic_or ;

logic_or      -> logic_and ( "or" logic_and )* ;
logic_and     -> equality ( "and" equality )* ;
equality      -> comparison ( ( "!=" | "==" ) comparison )* ;
comparison    -> term ( ( ">" | ">=" | "<" | "<=" ) term )* ;
term          -> factor ( ( "-" | "+" ) factor )* ;
factor        -> unary ( ( "/" | "*" ) unary )* ;

unary         -> ( "!" | "-" ) unary | call ;
call          -> primary ( "(" arguments? ")" | "." IDENTIFIER )* ;
primary       -> "true" | "false" | "this" | instanceof | cast
               | NUMBER | STRING | IDENTIFIER | "(" expression ")" ;

instanceof    -> "instanceof" "(" expression "," TYPE ")" ; 
cast          -> TYPE "(" expression ")" ;
```

## Utility Rules

```ebnf
function      -> IDENTIFIER "(" parameters? ")" ":" block "end" ;
parameters    -> IDENTIFIER ( "," IDENTIFIER )* ;
arguments     -> expression ( "," expression )* ;
```

## Lexical Grammar

```ebnf
NUMBER        -> DIGIT+ ( "." DIGIT+ )? ;
STRING        -> "\"" <any char except "\"">* "\"" ;
IDENTIFIER    -> ALPHA ( ALPHA | DIGIT )* ;
ALPHA         -> "a" ... "z" | "A" ... "Z" | "_" ;
DIGIT         -> "0" ... "9" ;
TYPE          -> "int" | "float" | "bool" | "string" | "char" ;
```

## Keywords

```ebnf
keyword       -> class 
              | func 
              | for 
              | while 
              | end 
              | if 
              | true 
              | false 
              | this
              | null
              | var
              | final
```
