# Overview

I figured it might help to note down the foundational concepts recognized by the `invariants` syntax oracle. To fully understand the DSL, it is best to view it broken down into the following architectural concepts:

## 1. Statements

Statements act as the structural scaffolding of the DSL. They do not evaluate to a value themselves; rather, they declare contracts, bind scope, and instruct the oracle on _when_ to execute a rule (ie; defining the semantic anchors). This encompasses the core keywords of the language, such as `spec`, `field`, `check`, and `invariant`.

More details about declarations and scope can be found in [statements.md](statements.md).

## 2. Expressions

If statements are the scaffolding, expressions are the actual computational payload. An expression is any segment of code—built from operators, identifiers, and relational logic—that evaluates to a distinct value. These are what ultimately get transpiled directly into C++ lambdas.

More details about operator precedence and implicit identifiers (like `this` and `value`) can be found in [expressions.md](expressions.md).

## 3. Data Types

`invariants` aims to maintain parity with the OpenAPI 3.0 spec. As the lexer reads atomic literals (like `500` or `"USD"`), the syntax oracle maps these directly to foundational C++ types to ensure high performance during the decoding loop. This also covers how complex, recursive structures like arrays and objects are handled.

More details about data types and bounds can be found in [data_types.md](data_types.md).

## 4. Comments

The DSL uses standard C-style double slashes (`//`) for single-line comments.

> Example: `// Ensure the total price includes the bulk discount`
