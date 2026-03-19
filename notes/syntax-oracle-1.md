# Syntax Oracle Design

Our first goal is to build a high-performance "syntax oracle" system to
constrain LLM generation to a structured output space via a sort of "annotated
DSL". Thus, as outlined in my proposal, we can define the syntax oracle as:

$$
O = \langle M_{syn}, E_{sem} \rangle
$$

- $M_{syn}$ is the syntactic module, a deterministic finite automaton
  derived from the grammar to handle token-level masking.
- ${E_{sem}$ is the semantic module, a dependancy graph that evaluates
  cross-field invariants (ie; context-_sensitive_ constraints) that a CFG cannot
  cpature.

## The Syntactic Module

At every single step of text generation, $M_{syn}$ will look at the current
state and ask "which tokens in the model's vocabulary are _legally_ allowed to
come next?". For instance, if the model is inside a numeric field, $M_{syn}$
generates a bitmask to only allow things like digits and separators.

## The Semantic Module

Because this operates in a context-sensitive manner, we cannot run this at every
step. Thus, it operates whenever $M_{syn}$ indicates a field is complete. This
is to avoid pre-maturely rejecting a value (ie; invariant is x>300, model is
generating 400, but it gets stopped at 40). In any case, it runs a logical check
using the syntax oracle to see if the value is legal.

## Module Switching

In our DSL, we will define "semantic anchors", which will signal the oracle to
switch from the syntactic module to the semantic. For instance:

1. When it transitions across a "field boundary", an interrupt is triggered (ie;
   the model finished typin the avlue for a certain field)
2. The oracle checks the semantic module's dependency graph - are all variables
   for this invariant populated?
3. If so, it can validate the field. If not, wait till the dependency 
   requirements are met.

## Syntax

For now, we will constrain ourselves to something to complement an OpenAPI spec.
It shouldn't define the data structure, that job belongs to OpenAPI, but rather
the contracts between data points. We might be able to use a syntax similar to
`jq`, so we can match fields more deeply than just on the last name (ie;
Contract.Transaction.Amount vs Amount, which might apply to many different
fields).

Ideally, we would be able to import a JSON schema, and then define a spec which
refers to certain fields within the spec.

### Example

Let's say we're building an LLM agent that generates bulk orders for some
shipment. Each `BulkOrder` object will have a `unit_price`, `quantity`,
`total_price`, and `currency`.

The spec might look like this in JSON:
```json
{
  "openapi": "3.0.0",
  "components": {
    "schemas": {
      "BulkOrder": {
        "type": "object",
        "properties": {
          "unit_price": { "type": "number" },
          "quantity": { "type": "integer" },
          "total_price": { "type": "number" },
          "currency": { "type": "string" }
        }
      }
    }
  }
}
```

And our spec might look like this:

```
spec BulkOrder {
    field unit_price {
        check: value > 0.0;
    }

    field quantity {
        check: value >= 1;
        check: value <= 1000;
    }

    field currency {
        check: value in ["USD", "EUR", "GBP"];
    }

    invariant valid_total_price {
        this.total_proce == this.unit_price * this.quantity;
    }

    invariant bulk_discount {
        this.quantity > 500 -> this.total_price < (this.unit_price *
        this.quantity);
    }
}
```

Thus, in our spec, `field` indicates a _scoped_ predicate. `invariant`, is,
well, an invariant. Invariants are context-sensitive, whereas fields are
context-free. As such, field blocks are essentially just refinement types, AKA,
a type endowed with a predicate.

# Resources

- https://medium.com/@docherty/controlling-your-llm-deep-dive-into-constrained-generation-1e561c736a20
- https://openai.com/index/introducing-structured-outputs-in-the-api/
- https://medium.com/better-programming/testing-out-llama-cpp-grammar-constraint-based-sampling-f154e48e6028
- https://blog.dottxt.co/coalescence.html
- https://arxiv.org/abs/2307.09702
- https://arxiv.org/abs/2411.15100
