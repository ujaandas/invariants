import numpy as np
import invariants
from invariants.Buffer import FieldBuffer


class ConstraintProcessor:
    def __init__(
        self, runtime: invariants.Runtime, buffer: FieldBuffer, top_k: int = 1000
    ):
        self.runtime = runtime
        self.buffer = buffer
        self.top_k = top_k

    def __call__(self, input_ids: list[int], scores: np.ndarray) -> np.ndarray:
        # Mask -infty
        masked_scores = np.full_like(scores, -np.inf)

        # Get indices of most likely tokens
        # Argpartition much faster than full sort
        top_k_indices = np.argpartition(scores, -self.top_k)[-self.top_k :]

        # Only validate top candidates
        valid_tokens_found = 0

        for token_id in top_k_indices:
            proposed_str = self.buffer.speculative_decode(token_id)
            status = self.runtime.validate_active_field_partial(proposed_str)

            # If OK, restore original probability
            if status != invariants.ValidationStatus.Invalid:
                masked_scores[token_id] = scores[token_id]
                valid_tokens_found += 1

        if valid_tokens_found == 0:
            # TODO: Expand top_k search here
            print(
                f"\nDead end reached for field '{self.runtime.get_active_field_name()}'. Halting."
            )

        return masked_scores
