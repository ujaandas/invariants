# Contributing & Development Guide

This is a solo monorepo, but discipline here writes the thesis methodology chapters later[^1]! The `main` branch is sacred and must always pass 100% of its tests.

[^1]: Or so I've been told.

## 1. Employ TDD!
Do not write any C++ without a failing test. Use red/green testing. 
1. Write the GTest in `lang/tests/*` first and watch it fail.
2. Write the absolute bare minimum C++ to pass it.
3. Clean up memory stuff, optimize, and document.

## 2. Branching & PRs
Always use branches and PRs to trigger GitHub Actions and Copilot reviews. 

**Format:** `type/scope/short-description`
* **Types:** `feat`, `fix`, `refactor`, `docs`, `test`, `chore`
* **Scopes:** `lang`, `bridge`, `py`

> #### Example branch name: `feat/parser/add-binary-expressions`

*Merge Strategy: Squash and merge into `main`.*

## 3. Project Map
* `lang/`: The `invariants` DSL and syntax oracle.
* `bindings/`: Python/C++ FFI.
* `python/`: The `invariants` Python package and LLM decoding loop.
* `docs/`: Notes and drafts.