import invariants_cpp  # noqa: N999
import numpy as np

from invariants.Buffer import FieldBuffer
from invariants.Engine import Engine


class ConstraintProcessor:
    def __init__(
        self,
        runtime: invariants_cpp.Runtime,
        buffer: FieldBuffer,
        engine: Engine,
        top_k: int = 1000,
    ):
        self.runtime = runtime
        self.buffer = buffer
        self.engine = engine
        self.top_k = top_k
        self.eos_token = engine.llm.token_eos()

    def __call__(self, input_ids: list[int], scores: np.ndarray) -> np.ndarray:
        # 1. Identify the top K candidates
        top_k_indices = np.argpartition(scores, -self.top_k)[-self.top_k :]
        top_k_strings = [self.engine.vocab_strings[idx] for idx in top_k_indices]
        current_text = self.buffer.current_text()

        # 2. CRITICAL FIX: Mask ALL tokens outside of our top_k to -inf.
        # This guarantees the sampler can never pick unmapped tensor padding
        mask = np.ones(scores.shape, dtype=bool)
        mask[top_k_indices] = False
        scores[mask] = -np.inf

        # 3. C++ evaluates the remaining top_k and masks invalid ones in-place
        invariants_cpp.process_logits_batch(
            self.runtime, scores, top_k_indices.tolist(), top_k_strings, current_text
        )

        # 4. Failsafe: If C++ rejected every single token in the top_k,
        # force an EOS token so the engine gracefully ends the field.
        if np.max(scores) == -np.inf:
            scores[self.eos_token] = 0.0

        return scores
