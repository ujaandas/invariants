# invariants

Constrained LLM generation via semantic invariants, topological dependency graphs, and refinement types.

`invariants` is a high-performance constrained decoding framework centered around **global semantic constraints and deterministic mathematical cascades**, rather than purely syntactic text formatting. While existing grammar-based approaches (such as context-free grammars or JSON schemas) excel at enforcing local structure, they fundamentally cannot express relationships between values generated at different points in a document, nor can they dynamically compute fields to save GPU compute.

`invariants` bridges a high-performance compiler and runtime with Python (`pybind11`), using a topological dependency engine (Kahn's algorithm) to sequence fields, validate incremental character streaming via zero-copy logit masking, and automatically bypass the LLM entirely for mathematically determined variables.

## Overview

The project consists of three major layers:

* **Compiler Pipeline (`lang/`)** - Custom Lexer, Parser, two-pass Binder, and Dependency Analyzer that parse the specification DSL into an AST.
* **C++ Runtime Engine (`invariants_runtime`)** - Manages symbol resolution, dynamic environment state, invariant validation, and deterministic value solving.
* **Python Orchestration (`python/` & `bindings/`)** - Integrates seamlessly with inference backends (like `llama.cpp`) to intercept logits in-place and manage stateful JSON construction.

## What makes this different?

Traditional constrained decoding systems operate on **local information**. They ensure that the next token matches a regular expression or a JSON type, but they are blind to global context.

For example, constraints such as:

* `total_price == unit_price * quantity`
* `quantity > 500 -> total_price < unit_price * quantity`
* Cross-field specifications where nested objects (`profile.vcpu_cores`) dictate downstream limits

require knowledge of previously generated fields. These are **global semantic constraints**.

`invariants` solves this by constructing a topological dependency graph across single or multi-spec modules. During autoregressive generation:

1. **Topological Ordering:** Kahn's algorithm resolves field execution order, handling forward references seamlessly.
2. **Deterministic Bypasses:** When a field's value becomes mathematically constrained by upstream fields, the runtime computes it instantly in C++ and **bypasses the LLM entirely**, reducing GPU token generation overhead.
3. **In-Memory Logit Masking:** For unmasked fields, incoming logits are evaluated and filtered in-place to prevent invalid tokens or out-of-bounds hallucinations.


**Module & Specification Structure**

* A module is constructed from one or more `spec` declarations.
* Each `spec` contains an identifier and a block of members enclosed in braces `{}`.
* Members can be either `field` or `invariant` declarations.
* **Fields:** Declared with `field <identifier>: <Type> { <constraints> }`.
* **Invariants:** Declared with `invariant <identifier> { <constraints> }`.
* **Constraints:** Expressions inside a field or invariant block. Every constraint must be explicitly terminated with a semicolon (`;`).

**Supported Data Types**

* **Primitives:** `Number` (floating-point), `Integer` (whole numbers), `String`, `Boolean`, and `Null`.
* **Collections:** `Array<Type>` and `Map<KeyType, ValueType>`.
* **Custom Objects:** Any previously declared `spec` identifier can be referenced directly as a field type.

**Literals & Identifiers**

* **Booleans:** `true` and `false`.
* **Null:** The `null` keyword.
* **Numbers:** Integers (e.g., `67`) and decimals (e.g., `6.7`).
* **Strings:** Text enclosed in quotes.
* **Lists:** Comma-separated expressions enclosed in brackets (e.g., `[a, b, c]`).
* **Identifiers:** Standard variable and spec names (e.g., `FooBar`), used to reference fields or types.

**Operators & Expressions**

* **Arithmetic:** `+` (Add), `-` (Subtract/Negate), `*` (Multiply), `/` (Divide), `%` (Modulo).
* **Comparison:** `==` (Equal), `!=` (Not Equal), `<` (Less Than), `<=` (Less/Equal), `>` (Greater Than), `>=` (Greater/Equal).
* **Logical:** `&&` (And), `||` (Or), `!` (Not).
* **Implication:** `->` computes conditional consequence (e.g., `A -> B`).
* **Membership:** `IN` (In collection), `NIN` (Not In collection), and `NI` (Contains).
* **Member Access:** The dot `.` operator for traversing object fields.
* **Indexing:** Bracket notation `[index]` for targeting elements in arrays or maps.
* **Grouping:** Parentheses `(expr)` for overriding mathematical precedence.

**Reserved Context Keywords**

* **`this`:** References the current schema scope, primarily used to target cross-field paths within invariant blocks (e.g., `this.profile.vcpu_cores`).

## Example

An enterprise-grade multi-spec example showcasing nested profiles, topological reordering, and multi-tier mathematical cascades:

```text
spec NodeProfile {
    field vcpu_cores: Number {}
    field ram_gb: Number {}
    
    invariant node_hardware_balance {
        this.vcpu_cores >= 2.0;
        this.vcpu_cores <= 64.0;
        this.ram_gb >= this.vcpu_cores * 2.0;
        this.ram_gb <= 256.0;
    }
}

spec CloudClusterQuote {
    // Declared first to mandate topological reordering via Kahn's algorithm
    field total_monthly_bill: Number {
        value == this.compute_cost + this.storage_cost;
    }
    
    field compute_cost: Number {
        value == (this.profile.vcpu_cores * 15.0 + this.profile.ram_gb * 4.0) * this.node_count;
    }
    
    field storage_cost: Number {
        value == this.storage_tb * 65.0;
    }

    field node_count: Number {}
    field profile: NodeProfile {}
    field storage_tb: Number {}

    invariant cluster_scale {
        this.node_count >= 2.0;
        this.node_count <= 16.0;
    }
}

```
## Python Bindings (`invariants_cpp`)

The project exposes native Python bindings via `pybind11`, embedding the high-performance C++ execution engine directly inside Python inference loops. This architecture ensures that Python is only used for high-level orchestration and model interfacing, while the heavy lifting of AST parsing, dependency resolution, and string evaluation remains in compiled C++.

The `invariants_cpp` module exposes the following core capabilities:

* **`EngineSession(source, root_spec)`:** Compiles the DSL into an Abstract Syntax Tree (AST), binds symbols, and executes Kahn's topological sort to build a dependency-aware `Runtime` execution schedule.
* **Zero-Copy Logit Masking:** The `process_logits_batch` function accepts a raw `numpy` array of LLM logits. It evaluates the string representations of the `top_k` proposed tokens against the active C++ constraints. Invalid tokens are immediately overwritten with `-infinity` directly in memory, preventing the sampler from selecting them without requiring data to be copied back and forth between Python and C++.
* **State Machine Navigation:** Exposes the `Runtime` to Python, allowing the orchestrator to query the C++ engine for the next required field (`get_active_field_name`), verify if it can be skipped (`is_active_field_deterministic`), and commit generated values (`submit_val_str`).
* **Automatic JSON Formatting:** The C++ bindings automatically detect structural exit characters (`,`, `\n`, `}`) and trim string quotes (`"`, `'`), abstracting JSON syntax management away from the Python layer.


## Using the Library in Practice

In practice, you do not need to manually manage the C++ state machine or the logit-masking loop. The `invariants.Processor.ConstrainedGenerator` helper class abstracts the entire generation cycle into a single function call.

Below is an enterprise-grade example demonstrating a multi-spec execution graph. The engine will parse the schema, calculate the optimal execution order, prompt the LLM, and automatically bypass the LLM for the mathematically determined cost fields.

```python
from invariants.Engine import Engine
from invariants.Processor import ConstrainedGenerator


def main():
    source = """
    spec NodeProfile {
        field vcpu_cores: Number {}
        field ram_gb: Number {}

        invariant node_hardware_balance {
            this.vcpu_cores >= 2.0;
            this.vcpu_cores <= 64.0;
            this.ram_gb >= this.vcpu_cores * 2.0;
            this.ram_gb <= 256.0;
        }
    }

    spec StorageProfile {
        field storage_tb: Number {}
        field redundancy: String {
            value == "Geo-Redundant";
        }

        invariant capacity_limits {
            this.storage_tb >= 1.0;
            this.storage_tb <= 100.0;
        }
    }

    spec CloudClusterQuote {
        // Declared first to mandate Kahn's topological reordering
        field total_monthly_bill: Number {
            value == this.compute_cost + this.storage_cost;
        }

        field compute_cost: Number {
            value == (this.profile.vcpu_cores * 15.0 + this.profile.ram_gb * 4.0) * this.node_count;
        }

        field storage_cost: Number {
            value == this.storage.storage_tb * 65.0;
        }

        field node_count: Number {}
        field profile: NodeProfile {}
        field storage: StorageProfile {}

        invariant cluster_scale {
            this.node_count >= 2.0;
            this.node_count <= 16.0;
        }
    }
    """

    print("Initializing LLM Engine...")

    # 1. Load your local LLM (e.g., via llama.cpp)
    engine = Engine()

    # 2. Attach the engine to the Constrained Generator orchestrator
    generator = ConstrainedGenerator(engine)

    system_prompt = (
        "You are an automated provisioning orchestrator configuring an enterprise Kubernetes cluster quote. "
        "Output ONLY valid JSON."
    )

    print("\nRunning Generator...")

    # 3. Execute the constrained graph
    result = generator.generate(
        source, "CloudClusterQuote", system_prompt, verbose=True
    )

    # 4. Analyze the runtime benchmark metrics
    print("\033[1m--- Generation Benchmark Metrics ---\033[0m")
    print(f"Total Fields Resolved:       {result.total_fields}")
    print(f"Fields Bypassed (Zero GPU):  {result.fields_bypassed}")
    print(f"LLM Tokens Sampled:          {result.tokens_sampled}")
    print(f"Total Generation Wall Time:  {result.wall_time_seconds:.3f}s")

    if result.wall_time_seconds > 0:
        print(
            f"Constrained Decode Speed:    {result.tokens_sampled / result.wall_time_seconds:.2f} tok/s"
        )


if __name__ == "__main__":
    main()
```


## Prerequisites

[Nix](https://nixos.org/download/) with flakes enabled is highly recommended. All compilers, build tools, and test dependencies are provided automatically by the flake.


## Getting started

### Enter the development shell

```bash
nix develop

```

### Configure the local build directory

Run the configure step once (and again whenever `CMakeLists.txt` changes):

```bash
nix run .#configure

```

### Run the test suite

```bash
nix run .#test

```

## Project layout

```text
lang/         Compiler frontend, lexer, parser, binder, analyzer, and runtime core
bindings/     pybind11 native bindings for the C++ runtime
python/       Python orchestration wrappers, buffer tracking, and processor hooks
docs/         Design notes and architecture documentation

```

See [CONTRIBUTING.md](https://www.google.com/search?q=CONTRIBUTING.md) for branching conventions, guidelines, and merge strategy.