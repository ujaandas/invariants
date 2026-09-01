import numpy as np
import invariants_cpp
from invariants.Buffer import FieldBuffer

class ConstraintProcessor:
    def __init__(self, runtime: invariants_cpp.Runtime, buffer: FieldBuffer, engine, top_k: int = 1000):
        self.runtime = runtime
        self.buffer = buffer
        self.engine = engine
        self.top_k = top_k

    def __call__(self, input_ids: list[int], scores: np.ndarray) -> np.ndarray:
        # Get the highest probability indices
        top_k_indices = np.argpartition(scores, -self.top_k)[-self.top_k:]
        
        # Instantly fetch pre-decoded strings from the Engine
        top_k_strings = [self.engine.vocab_strings[idx] for idx in top_k_indices]
        
        # Get current string state
        current_text = self.buffer.current_text()
        
        # ONE C++ call to validate and mask the numpy array in-place
        invariants_cpp.process_logits_batch(
            self.runtime, 
            scores, 
            top_k_indices.tolist(), 
            top_k_strings, 
            current_text
        )
        
        return scores