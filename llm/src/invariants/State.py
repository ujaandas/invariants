from dataclasses import dataclass
import torch


@dataclass
class DecodeState:
    past_kv: any
    logits: torch.tensor
    generated: list[int]
