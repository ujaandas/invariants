from dataclasses import dataclass
import torch
from typing import Any


@dataclass
class DecodeState:
    past_kv: Any
    logits: torch.Tensor
    generated: list[int]
