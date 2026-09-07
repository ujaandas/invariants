from invariants.Engine import Engine

# Tracks tokens for the active field so multi-token UTF-8 chars decode correctly


class FieldBuffer:
    def __init__(self, engine: Engine):
        self.engine = engine
        self.token_ids: list[int] = []

    def commit_token(self, token: int):
        self.token_ids.append(token)

    def speculative_decode(self, candidate_token: int) -> str:
        return self.engine.decode(self.token_ids + [candidate_token])

    def current_text(self) -> str:
        return self.engine.decode(self.token_ids)

    def clear(self):
        self.token_ids.clear()

    def __len__(self):
        return len(self.token_ids)
