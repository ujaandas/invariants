# invariants

Constrained LLM generation via semantic invariants and refinement types.

`invariants` is a research project exploring how to guide large language model
decoding by encoding semantic constraints as refinement types and verifiable
invariants. The core runtime is written in C++ (`lang/`), with Python bindings
planned for integration into LLM inference pipelines.

> **Status:** early-stage — build tooling and project skeleton are in place;
> core language features are under active development.

---

## Prerequisites

[Nix](https://nixos.org/download/) with flakes enabled is the only hard
requirement. All compilers, build tools, and test dependencies are provided by
the flake.

<details>
<summary>Enable flakes if you haven't already</summary>

Add the following to `~/.config/nix/nix.conf` (or `/etc/nix/nix.conf`):

```
experimental-features = nix-command flakes
```

</details>

---

## Getting started

### Enter the development shell

```bash
nix develop
```

This drops you into a shell with Clang, CMake, Ninja, GoogleTest,
`clang-tools`, `cppcheck`, and `prek` available.

### Configure the local build directory

Run the configure step once (and again whenever `CMakeLists.txt` changes):

```bash
nix run .#configure
```

This generates `.nix-dev/build/` with a `compile_commands.json` that both
clangd and the `clang-tidy` pre-commit hook read.

---

## Building & testing

| Task | Command |
|---|---|
| Build & run all tests (Nix sandbox) | `nix build` |
| Run the `hello_world` executable | `nix run` |
| Incremental build (inside devshell) | `cmake --build .nix-dev/build` |
| Run tests (inside devshell) | `ctest --test-dir .nix-dev/build --output-on-failure` |

CI runs `nix build` on every push and pull request targeting `main`.

---

## Pre-commit hooks

[`prek`](https://prek.j178.dev) is used for pre-commit automation.
Inside the dev shell, install the hooks once with:

```bash
prek install
```

The hooks run `clang-format` (Google style), `clang-tidy`, and `cppcheck` on
every commit. The `clang-tidy` hook requires the build directory to exist --
run `nix run .#configure` first if it doesn't.

---

## Project layout

```
lang/          C++ DSL runtime and syntax oracle (CMake project)
  src/         Production source files
  tests/       GoogleTest-based tests
bindings/      Python/C++ FFI (planned)
python/        Python package and LLM decoding loop (planned)
docs/          Notes and design drafts
```

See [CONTRIBUTING.md](CONTRIBUTING.md) for branching conventions, TDD
guidelines, and merge strategy.
