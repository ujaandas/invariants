# Building the Syntax Oracle Compiler

## Frontend

We need a lexer, parser and symbol table. The lexer will break our file into the
relevant tokens, like `spec`, `field`, `invariant`, etc... The parser will walk
the tokens and build an AST, and finally, the symbol table is how we store the
link between each of our fields and the actual OpenAPI properties.

### Grammar

Our DSL is probably LL(k), as our parser can just read from left-to-right, and
build our parse tree from top-down, or leftmost first. Ideally, we want to aim
for `LL(1)`, and it should be doable. For instance, if we see the word `spec`,
we are absolutely _sure_ we are starting a specification block. We'll try to
retain this by not "overloading" any operators or anything.

An example of our grammar might be something like:

```
Spec -> "spec" + Identifier + "{" + ( Field | Invariant )* + "}"
```

### Parser

We could probably get away with RD for the top-level grammar, because it aligns
with the inherent "hierarchy" that comes with RD. However, inside each field or
invariant, when we hit the expression, we might want to consider a different,
maybe simpler parser, like a Pratt parser.



# Resources
- https://matklad.github.io/2020/04/13/simple-but-powerful-pratt-parsing.html

