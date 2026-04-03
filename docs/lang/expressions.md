# Expressions

In the `invariants` DSL, an **expression** is any segment of code that evaluates to a distinct value. Because the syntax oracle transpiles the parsed Abstract Syntax Tree (AST) directly into C++ lambdas, expressions represent the actual computational payload of the contract.

Unlike statements (which define scope and structure), expressions are strictly mathematical and logical.

## Implicit Identifiers

Context dictates which variables are in scope during expression evaluation:

- **`value`**: Only available within a `field` block. Represents the localized, context-free value currently being evaluated by the syntax oracle.
- **`this`**: Only available within an `invariant` block. Acts as a pointer to the current `spec` object, allowing dot-notation access to fully populated sibling fields (e.g., `this.unit_price`).

## Operator Precedence

To prevent infinite left-recursion during parsing and to maintain standard mathematical rules, expressions follow a strict hierarchy of precedence (from lowest to highest):

### 1. Logical Implication (`->`)

Evaluates an "if-then" relationship. The right-hand expression is only enforced if the left-hand expression evaluates to `true`.

- Example: `this.quantity > 500 -> this.total_price < 1000.0;`

### 2. Equality (`==`, `!=`)

Checks for absolute equality or inequality between two evaluated expressions.

- Example: `this.currency == "USD"`

### 3. Comparison (`>`, `>=`, `<`, `<=`, `in`)

Evaluates relational bounds and set membership.

- Example: `value in ["A", "B"]`

### 4. Term & Factor (`+`, `-`, `*`, `/`, `%`)

Standard arithmetic operations. Operands are automatically promoted to floating-point where appropriate before the C++ lambda is executed.

- Example: `this.unit_price * this.quantity`

### 5. Primary Expressions

The atomic, indivisible units of the language. This includes literals (`500`, `"USD"`, `true`), implicit identifiers (`value`, `this`), member access (`this.quantity`), and grouped expressions enclosed in parentheses `()`.
