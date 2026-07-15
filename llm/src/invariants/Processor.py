import numpy as np
import invariants_cpp
from invariants.Buffer import FieldBuffer


class ConstraintProcessor:
    def __init__(
        self, runtime: invariants_cpp.Runtime, buffer: FieldBuffer, top_k: int = 1000
    ):
        self.runtime = runtime
        self.buffer = buffer
        self.top_k = top_k

    def __call__(self, input_ids: list[int], scores: np.ndarray) -> np.ndarray:
        masked_scores = np.full_like(scores, -np.inf)
        top_k_indices = np.argpartition(scores, -self.top_k)[-self.top_k :]

        valid_tokens_found = 0
        field_type = self.runtime.get_active_field_type()

        for token_id in top_k_indices:
            proposed_str = self.buffer.speculative_decode(token_id)

            # Clean spaces and check if  LLM trying to exit field
            clean_str = proposed_str.strip()
            is_exit_token = any(clean_str.endswith(c) for c in [",", "\n", "}"])

            # Strip JSON structural punctuation
            clean_str = clean_str.rstrip(",\n} ")

            # Strip outer JSON quote marks for str fields
            if field_type == invariants_cpp.FieldType.String:
                if clean_str.startswith('"') or clean_str.startswith("'"):
                    clean_str = clean_str[1:]
                if clean_str.endswith('"') or clean_str.endswith("'"):
                    clean_str = clean_str[:-1]

            # Validation
            if field_type == invariants_cpp.FieldType.String and clean_str == "":
                status = invariants_cpp.ValidationStatus.PartialValid
            else:
                status = self.runtime.validate_active_field_partial(clean_str)

            # Masking Decision
            if status != invariants_cpp.ValidationStatus.Invalid:
                if is_exit_token:
                    # If exit token, val MUST be fully valid
                    if status == invariants_cpp.ValidationStatus.Valid:
                        masked_scores[token_id] = scores[token_id]
                        valid_tokens_found += 1
                else:
                    masked_scores[token_id] = scores[token_id]
                    valid_tokens_found += 1

        if valid_tokens_found == 0:
            print(
                f"\n[Warning] Dead end reached for field '{self.runtime.get_active_field_name()}'. Halting."
            )

        return masked_scores
