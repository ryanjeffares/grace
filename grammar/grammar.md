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
varDecl       -> "var" TYPE IDENTIFIER ( "=" expression )? ";" ;
finalDecl     -> "final" TYPE IDENTIFIER "=" expression ";" ;
```

## Statements

```ebnf
statement     -> exprStmt
               | forStmt
               | ifStmt
               | printStmt
               | printLnStmt
               | returnStmt
               | whileStmt 
               | block ; 

exprStmt      -> expression ";" ;
forStmt       -> "for" IDENTIFIER "in" RANGE ( "by" NUMBER ) ":"
                 block 
                 "end" ;
ifStmt        -> "if" expression ":" 
                 block
                 ( "else" "if" expression ":" block )*
                 ( "else" ":" block )
                 "end" ;
printStmt     -> "print" "(" expression ")" ";" ;
printLnStmt   -> "println" "(" expression ")" ";" ;
returnStmt    -> "return" expression? ";" ;
whileStmt     -> "while"  expression ":" block "end" ;
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
NUMBER        -> INTEGER | FLOAT ;
INTEGER       -> DIGIT+ ;
FLOAT         -> DIGIT+ "." DIGIT+ ;
STRING        -> "\"" <any char except "\"">* "\"" ;
IDENTIFIER    -> ALPHA ( ALPHA | DIGIT )* ;
ALPHA         -> "a" ... "z" | "A" ... "Z" | "_" ;
DIGIT         -> "0" ... "9" ;
TYPE          -> "int" | "float" | "bool" | "string" | "char" ;
RANGE         -> ( NUMBER | IDENTIFIER ) ".." ( NUMBER | IDENTIFIER ) ;
```

## Keywords

```ebnf
keyword       -> class 
              | func 
              | for 
              | in 
              | by
              | while 
              | end 
              | if
              | else 
              | true 
              | false 
              | this
              | null
              | var
              | final
```
