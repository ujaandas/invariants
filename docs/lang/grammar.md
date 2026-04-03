# Lexical Grammar & Tokens

I figured it might help to note down the foundational tokens recognized by the `invariants` lexer.

## 1. Keywords

Reserved words that dictate structure and logic.

- `spec`: declares a top-level schema contract.
- `field`: declares a _scoped_ (to a certain field, annotated by `jq`-like syntax) schema contract.
- `check`: evaluates a given **context-free** predicate (ie; refinement type), and is linked to a certain field.
- `invariant`: declares a **context-sensitive** predicate, and is linked to multiple fields by name.

## 2. Identifiers

(User-defined) Names for structures and variables, defined by `[a-zA-Z_][a-zA-Z0-9_]*`.

Some names are reserved to refer to higher-level contracts, such as `this`, which refers to a field associated with the current top-level schema.

## 3. Literals/Types

- **Numbers:** Integers and floating-point values (e.g., `1`, `0.0`, `500`).
- **Strings:** Text enclosed in double quotes (e.g., `"USD"`).
- **Booleans:**: Boolean values, so either `true` or `false`.

## 4. Operators & Symbols

- **Structural:** `{`, `}`, `:`, `;`, `[`, `]`, `,`
- **Relational:** `>`, `>=`, `<=`, `<`, `==`, `!=`
- **Mathematical:** `*`, `+`, `-`, `/`
- **Logical:** `->` (Implication)
