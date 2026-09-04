import time
from dataclasses import dataclass

import invariants_cpp
import numpy as np

from invariants.Buffer import FieldBuffer
from invariants.Engine import Engine


@dataclass
class GenerationResult:
    json_output: str
    tokens_sampled: int
    fields_bypassed: int
    total_fields: int
    wall_time_seconds: float


class ConstraintProcessor:
    def __init__(self, runtime, buffer, engine):
        self.runtime = runtime
        self.buffer = buffer
        self.engine = engine
        self.eos_token = engine.llm.token_eos()

    def __call__(self, input_ids: list[int], scores: np.ndarray) -> np.ndarray:
        current_text = self.buffer.current_text()

        # C contig mem
        scores_contiguous = np.ascontiguousarray(scores, dtype=np.float32)

        # Mutate the contiguous array in C++
        invariants_cpp.mask_logits_full_vocab(
            self.runtime,
            scores_contiguous,
            self.engine.vocab_strings,
            current_text,
            False,
        )

        # If the mask successfully rejected EVERYTHING
        if not np.any(np.isfinite(scores_contiguous)):
            print(
                f"\n[!] FATAL: Mask rejected the entire vocabulary! Buffer: {current_text!r}"
            )
            # We force EOS here to stop generation rather than crashing llama.cpp with NaNs
            scores_contiguous[self.eos_token] = 0.0

        # Return the mutated contiguous array
        return scores_contiguous


class ConstrainedGenerator:
    def __init__(self, engine: Engine):
        self.engine = engine

    def generate(
        self, dsl_source: str, root_spec: str, system_prompt: str, verbose: bool = True
    ) -> GenerationResult:
        session = invariants_cpp.EngineSession(dsl_source, root_spec)
        rt = session.runtime

        final_json = "{\n"
        if verbose:
            print("\n\033[1m[Starting Constrained Execution Graph]\033[0m\n{")

        tokens_sampled = 0
        fields_bypassed = 0
        total_fields = 0

        start_time = time.perf_counter()

        while rt.has_more_fields():
            field_name = rt.get_active_field_name()
            total_fields += 1

            final_json += f'  "{field_name}": '
            if verbose:
                print(f'  "{field_name}": ', end="", flush=True)

            if rt.is_active_field_deterministic():
                fields_bypassed += 1
                val_str = rt.solve_deterministic()

                if (
                    val_str in ("true", "false")
                    or val_str.replace(".", "", 1).isdigit()
                ):
                    json_val = val_str
                else:
                    json_val = f'"{val_str}"'

                final_json += json_val
                if verbose:
                    print(f"{json_val}  \033[92m[C++ Bypassed]\033[0m", flush=True)

            else:
                buffer = FieldBuffer(self.engine)
                processor = ConstraintProcessor(rt, buffer, self.engine)
                state = self.engine.prefill(
                    f"{system_prompt}\n{final_json}", logits_processor=processor
                )

                generated_val = ""
                while True:
                    token = self.engine.step(state)
                    if token is None or token < 0 or token >= self.engine.llm.n_vocab():
                        break

                    tokens_sampled += 1
                    char_chunk = self.engine.decode([token])

                    exit_chars = [",", "}"]
                    if any(c in char_chunk for c in exit_chars):
                        for c in exit_chars:
                            if c in char_chunk:
                                char_chunk = char_chunk.split(c)[0]
                                break

                        generated_val += char_chunk
                        if verbose:
                            print(char_chunk, end="", flush=True)
                        break

                    buffer.commit_token(token)
                    generated_val += char_chunk
                    if verbose:
                        print(char_chunk, end="", flush=True)

                final_json += generated_val
                clean_val = generated_val.strip().rstrip(",\n} ")
                if not clean_val:
                    raise RuntimeError(
                        f"LLM generated an empty value for field '{field_name}'. "
                        "Logit constraint mask prevented invalid tokens, but the model terminated generation early."
                    )
                rt.submit_val_str(field_name, clean_val)
                if verbose:
                    print("  \033[94m[LLM Sampled]\033[0m", flush=True)

            if rt.has_more_fields():
                final_json += ",\n"
                if verbose:
                    print(",")
            else:
                final_json += "\n"
                if verbose:
                    print()

        final_json += "}"
        if verbose:
            print("}\n")

        wall_time = time.perf_counter() - start_time
        return GenerationResult(
            json_output=final_json,
            tokens_sampled=tokens_sampled,
            fields_bypassed=fields_bypassed,
            total_fields=total_fields,
            wall_time_seconds=wall_time,
        )
