# invariants

Constrained LLM generation via semantic invariants and refinement types.

`invariants` is a constrained decoding framework centered around **global semantic constraints**,
rather than purely syntactic ones. While existing grammar-based
approaches excel at enforcing local structure (JSON grammars, regular
expressions, context-free grammars, etc...), they fundamentally cannot express
relationships between values generated at different points in a document.

`invariants` extends this model by allowing schemas to define **cross-field semantic invariants**.
These invariants are compiled into a dependency graph,
allowing the runtime to evaluate global constraints incrementally during
autoregressive decoding. As long as the dependency graph is acyclic, downstream
fields are only generated once all required upstream values have been resolved.

## Overview

The project consists of two major components:

- **`invariants`** - an OpenAPI-inspired specification language for expressing
  refinement types and semantic invariants.
- **`invariants_runtime`** - a high-performance C++ runtime responsible for
  dependency resolution, validation, and deterministic evaluation.
- **`invariants_cpp`** - Python bindings for integrating the
  runtime into existing LLM inference pipelines.

## What makes this different?

Traditional constrained decoding systems operate on **local information**. They
can ensure that the next token satisfies a grammar or regular expression, but
cannot reason about relationships between values elsewhere in the generated
document.

For example, constraints such as:

- `total_price == unit_price * quantity`
- `quantity > 500 -> total_price < unit_price * quantity`
- `subnet` determines the valid range of `ip_address`
- `tier == "enterprise"` whenever `monthly_spend > 1000`

require knowledge of previously generated fields. These are **global semantic constraints**,
not local syntactic ones.

`invariants` models these relationships as a dependency graph. During
generation, the runtime maintains a shared environment containing all previously
resolved values, allowing every newly generated field to be validated against
both its own local refinement constraints and arbitrary cross-field invariants.

As a consequence, fields whose values become fully determined by upstream
dependencies can be computed directly by the runtime instead of sampled from the
language model.

## Example

Schemas are written using an OpenAPI-inspired DSL with support for refinement
types and semantic invariants.

```text
spec BulkOrder {
    field unit_price: Number {
        value > 0.0;
    }

    field quantity: Integer {
        value >= 1;
        value <= 1000;
    }

    field currency: String {
        value in ["USD", "EUR", "GBP"];
    }

    field total_price: Number { }

    invariant valid_total_price {
        this.total_price == this.unit_price * this.quantity;
    }

    invariant bulk_discount {
        this.quantity > 500 ->
            this.total_price < (this.unit_price * this.quantity);
    }
}
```

The runtime extracts field dependencies from these invariants, computes a valid
generation order, and incrementally validates every generated value against the
current global state.

## Prerequisites

[Nix](https://nixos.org/download/) with flakes enabled is highly recommended.
All compilers, build tools, and test dependencies are provided by the flake.

However, you are free to build and compile it yourself with CMake.
Read through the `flake.nix` to get an understanding of build-time
requirements.

## Getting started

### Enter the development shell

```bash
nix develop
```

This drops you into a shell with all relevant developer tooling available.

### Configure the local build directory

Run the configure step once (and again whenever `CMakeLists.txt` changes):

```bash
nix run .#configure
```

This generates `.nix-dev/build/` with a `compile_commands.json` that both
`clangd` and the `clang-tidy` pre-commit hook read.

### Run the test suite

We also provide a Nix app to run the test suite automatically:

```bash
nix run .#test
```

This automatically generates the compilation database before running `ctest`
with the appropriate arguments.

> Both commands are intentionally impure-they are convenience wrappers around
> the normal build process.

## Building

| Task                       | Command     |
| -------------------------- | ----------- |
| Build project and tests    | `nix build` |
| Run the example executable | `nix run`   |

## Python bindings

The project exposes native Python bindings via `pybind11`.

The C++ runtime is designed to be embedded inside existing decoding pipelines,
allowing Python inference engines to delegate semantic validation and dependency
resolution to native code while remaining model-agnostic.

## Web demo

Build the browser version from `wasm/lib.cpp`:

```bash
nix run .#wasm-configure
```

Then serve the generated files:

```bash
nix run .#wasm-serve
```

Open `http://localhost:8080/` in your browser (or
`.nix-dev/wasm/index.html`, which is copied alongside the generated artifacts).

The served demo loads `invariants_wasm.js` and `invariants_wasm.wasm` from
`.nix-dev/wasm/`.

CI runs `nix build` on every push and pull request targeting `main`.

## Pre-commit hooks

[`prek`](https://prek.j178.dev) is used for pre-commit automation.

Inside the development shell, install the hooks once with:

```bash
prek install
```

The hooks run `clang-format` (Google style), `clang-tidy`, and `cppcheck` on
every commit.

The `clang-tidy` hook requires the build directory to exist. Run
`nix run .#configure` first if it doesn't (although this is normally handled
automatically by the hook chain).

## Project layout

```
lang/          Compiler frontend, parser, runtime, and DSL implementation
bindings/      pybind11 bindings for the native runtime
python/        Python orchestration library and decoding utilities
wasm/          Browser demo and Emscripten entrypoint
docs/          Design notes and architecture documentation
```

See [CONTRIBUTING.md](CONTRIBUTING.md) for branching conventions, TDD
guidelines, and merge strategy.
