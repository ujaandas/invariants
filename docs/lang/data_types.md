# Data Types

`invariants` aims to maintain parity with the OpenAPI 3.0 spec. As such, the
supported types are also largely the same. The typical complexity of managing
the custom "object" type defined by the OAS 3 is largely mitigated by the entire DSL
being focused around `spec`s, which are basically just objects. This therefore
enables first-class support thereof.

## Defining Types

As `field`s within the `spec` are essentially refinement types, type-checks and
whatnot are invariably linked to the associated `field` object. Thus, specifying
a type is as simple as follows:

`field Foo: FooType { ... }`

## Mixed Types

Mixed types (ie; to define a list with many possible types) are not supported.
Prefer defining another `spec` and set that as your list type.

## Numbers

Like the OAS, a "number" can either refer to an actual number, which includes
both integer and floating-point numbers, or just an integer. For simplicity,
all numbers are promoted to C++ foundational types, respectively.

### Minimum and Maximum

To define bounds for any number-typed `field`, simply write a constraint
expression inside the `field` block with the `<` or `>` operators. This enables
flexibility and removes any ambiguity (ie; minimum/maximum values in the OAS are
inclusive, so a field like `exclusiveMinimum: true` must be added for exclusivity).

### Multiples

Again, checking for multiples is just a matter of writing a constraint expression
using the `%` operator.

## Strings

Strings map directly to standard C++ `std::string`. In standard OpenAPI, strings are heavily constrained by `minLength`, `maxLength`, `enum`, and `pattern`. Within `invariants`, these constraints are implemented via standard expressions.

### Length Constraints

To bound string length, access the implicitly available `.length` property:

```
field username: String {
    this.value.length >= 3;
    this.value.length <= 20;
}
```

### Set Membership

Instead of relying on a dedicated `enum` array in the JSON schema, the DSL leverages the `in` operator to verify set membership mathematically:

```
field currency: String {
    this.value in ["USD", "GBP", "EUR"];
}
```

## Booleans

Booleans are strict binary types. The `invariants` scanner recognizes `true` and `false` as reserved keywords, evaluating them directly to C++ `bool` primitives. They can be used directly in logical implication statements without secondary comparisons.

## Null

To support OpenAPI's nullable fields, the DSL also recognizes `null` as a reserved keyword. This allows for native empty-state checks (eg; `this.value != null;`).

## Arrays

Arrays are homogeneous lists of elements. The `invariants` parser treats `Array` as a standard base type identifier, but allows for structural parameterization via bracket notation to define the inner type (eg; `Array<string>` or `Array<BulkOrder>`).

Similar to strings, bounds constraints like OAS's `minItems` and `maxItems` are handled dynamically via the `.length` property on the array value.

## Objects

Objects are not a primitive type in this DSL. Rather, an object is simply an instance of another top-level `spec`. This allows for deep, recursive schema nesting. If you need an object type, define a `spec` for it and reference it by its identifier.
