# invariants/Buffer.py
from invariants.Engine import Engine

# In BPE tokenizers, 1 token NEQ complete, valid string
# (e.g. sometimes, multi-byte UTF char is split across multiple tokens)
# FieldBuffer tracks tokens generated for curr active field and decodes them tgt


class FieldBuffer:
    def __init__(self, engine: Engine):
        self.engine = engine
        self.token_ids: list[int] = []

    def commit_token(self, token: int):
        """Permanently adds a generated token to the active field's buffer."""
        self.token_ids.append(token)

    def speculative_decode(self, candidate_token: int) -> str:
        """
        Temporarily appends a candidate token and decodes the entire sequence.
        This guarantees that multi-token UTF-8 characters are resolved correctly.
        """
        # Decode the entire buf + candidate
        return self.engine.decode(self.token_ids + [candidate_token])

    def current_text(self) -> str:
        """Returns the fully decoded text of the buffer so far."""
        return self.engine.decode(self.token_ids)

    def clear(self):
        """Clears the buffer when moving to a new JSON field."""
        self.token_ids.clear()

    def __len__(self):
        return len(self.token_ids)
