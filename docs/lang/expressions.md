# Expressions

In the `invariants` DSL, an **expression** is any segment of code that evaluates to a distinct value. Because the syntax oracle transpiles the parsed Abstract Syntax Tree (AST) directly into C++ lambdas, expressions represent the actual computational payload of the contract.

Unlike statements (which define scope and structure), expressions are strictly mathematical and logical.

## Implicit Identifiers

Context dictates which variables are in scope during expression evaluation:

- **`value`**: Only available within a `field` block. Represents the localized, context-free value currently being evaluated by the syntax oracle.
- **`this`**: Only available within an `invariant` block. Acts as a pointer to the current `spec` object. The language supports _recursive_ member access, allowing the parser to walk down deeply nested schemas via dot-notation (eg; `this.buyer.address.zipcode`).

## Operator Precedence

To prevent infinite left-recursion during parsing and to maintain standard mathematical rules, expressions follow a strict hierarchy of precedence (from lowest to highest):

### 1. Logical Implication (`->`)

Evaluates an "if-then" relationship. The right-hand expression is only enforced if the left-hand expression evaluates to `true`.

> Example: `this.quantity > 500 -> this.total_price < 1000.0;`

### 2. Equality (`==`, `!=`)

Checks for absolute equality or inequality between two evaluated expressions.

> Example: `this.currency == "USD"`

### 3. Comparison (`>`, `>=`, `<`, `<=`, `in`)

Evaluates relational bounds and set membership.

> Example: `value in ["A", "B"]`

### 4. Term & Factor (`+`, `-`, `*`, `/`, `%`)

Standard arithmetic operations. Operands are automatically promoted to floating-point where appropriate before the C++ lambda is executed.

> Example: `this.unit_price * this.quantity`

### 5. Primary Expressions

Atomic, indivisible units. Primary expressions bypass all operator precedence because they are self-contained values. This includes:

- Literals: Numbers (`500`, `0.0`), Strings (`"USD"`), and reserved keywords (`true`, `false`, `null`).
- Array Literals: Enclosed in brackets. Because they are primary expressions, they are valid anywhere a value is expected, not just alongside the `in` operator (e.g., `[1, 2, 3]`).
- Identifiers: Standard variables, implicit variables (`value`, `this`), and recursive member access (`this.quantity`).
- Groupings: Any expression tightly enclosed in parentheses `()`.
