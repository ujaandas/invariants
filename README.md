# MSc Dissertation - Formally Verified Structured Outputs in LLMs
This repository contains the LaTeX source for my MSc Computer Science dissertation at the University of Warwick.

It focuses on the intersection of formal verification models and their
applicability to large language models (LLMs). Specifically, it explores a
dual-process system that embeds an abstract machine into the LLM decoding loop
to enforce _semantically_ correct and type-safe agentic outputs.

## Repository Structure
Each document lives in its own folder. For instance:
* `proposal/`: Current working draft of the proposal.
* `template/`: Boilerplate LaTeX structure.

We also use Nix for reproducible build environments, containing the TeX Live
distribution and various other dependencies.

## Building the Documents
To compile any given document into a PDF, run:
```bash
nix build .#<document-folder-name>
```

For instance, if I wanted the PDF for my proposal, I would run:
```bash
nix build .#proposal
```

The resulting PDF and build artifacts will be available in the `result/`
symlink.

> For active writing with live-reloads, use the `watchdog` app I've packaged in
> the flake. When running it, ensure to give the relevant target source:
> ```bash
> nix run .#watchdog -- proposal
> ```
> Personally, I just use this and Vim + Tmux for any given doc. 
