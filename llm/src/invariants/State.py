from dataclasses import dataclass
from typing import Generator


@dataclass
class DecodeState:
    step_generator: Generator[
        int, None, None
    ]  # Llama.generate() yields integers (tokens)
    generated_tokens: list[int]
