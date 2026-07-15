from dataclasses import dataclass, field
from typing import Generator


@dataclass
class DecodeState:
    step_generator: Generator[
        int, None, None
    ]  # Llama.generate() yields integers (tokens)

    generated_tokens: list[int] = field(
        default_factory=list
    )  # Keep track of tokens generated in this field's window
