# invariants

Constrained LLM generation via semantic invariants and refinement types.

`invariants` aims to guide large language model decoding by encoding semantic
constraints as refinement types and verifiable invariants. 
The core runtime and DSL is written in C++ (`lang/`), with Python bindings
planned for integration into LLM inference pipelines.

## Prerequisites

[Nix](https://nixos.org/download/) with flakes enabled is highly recommended. 
All compilers, build tools, and test dependencies are provided by the flake.

However, you are free to build and compile it yourself with CMake. 
Read through the `flake.nix` to get an understanding of build-time requirements and whatnot.

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
clangd and the `clang-tidy` pre-commit hook read. This is also required for `clangd`, 
if you use it.

### Run test suite

We also provide a Nix app to run the test suite automatically:

```bash
nix run .#test
```

This also auto-generates the compilation database, but also runs `ctest` with the right arguments afterwards.

> Note that both of these are _impure_, and are no better than simple Bash scripts! Be careful!

## Building & testing

| Task | Command |
|---|---|
| Build & run all tests (Nix sandbox) | `nix build` |
| Run the `hello_world` executable | `nix run` |

### Web demo

Build the browser version from `wasm/lib.cpp`:

```bash
nix run .#wasm-build
```

Then serve the generated files and open the page in a browser:

```bash
nix run .#wasm-serve
```

The demo page is `wasm/index.html`, and it loads the generated
`invariants_wasm.js` and `invariants_wasm.wasm` artifacts from `.nix-dev/wasm/`.

CI runs `nix build` on every push and pull request targeting `main`.

## Pre-commit hooks

[`prek`](https://prek.j178.dev) is used for pre-commit automation.
Inside the dev shell, install the hooks once with:

```bash
prek install
```

The hooks run `clang-format` (Google style), `clang-tidy`, and `cppcheck` on
every commit. The `clang-tidy` hook requires the build directory to exist --
run `nix run .#configure` first if it doesn't (but this should be done automatically, 
as it itself is the first hook run!).

## Project layout

```
lang/          C++ DSL runtime and syntax oracle (CMake project)
  src/         Production source files
  tests/       GoogleTest-based tests
wasm/          Browser demo and Emscripten entrypoint
bindings/      Python/C++ FFI (planned)
python/        Python package and LLM decoding loop (planned)
docs/          Notes and design drafts
```

See [CONTRIBUTING.md](CONTRIBUTING.md) for branching conventions, TDD
guidelines, and merge strategy.
