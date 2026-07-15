from typing import cast
from transformers import (
    AutoTokenizer,
    AutoModelForCausalLM,
    BitsAndBytesConfig,
    PreTrainedTokenizerBase,
)
import torch

from invariants.State import DecodeState


class Engine:
    def __init__(
        self,
        model_name: str,
        tokenizer_name: str | None = None,
        device: str = "cuda",
        quantize: bool = True,
    ):
        self.device: str = device

        self.tokenizer = cast(
            PreTrainedTokenizerBase,
            AutoTokenizer.from_pretrained(tokenizer_name or model_name),
        )

        quant_config = None
        if quantize:
            quant_config = BitsAndBytesConfig(
                load_in_4bit=True,
                bnb_4bit_quant_type="nf4",
                bnb_4bit_use_double_quant=True,
                bnb_4bit_compute_dtype=torch.float16,
            )

        self.model = AutoModelForCausalLM.from_pretrained(
            model_name,
            quantization_config=quant_config,
            device_map=device,
        )

        self.model.eval()

    def prefill(
        self, input_ids: torch.Tensor, attn_mask: torch.Tensor | None = None
    ) -> DecodeState:
        # Ensure shape is [1, seq]
        if input_ids.dim() == 1:
            input_ids = input_ids.unsqueeze(0)

        # Load in GPU
        input_ids = input_ids.to(self.device)

        if attn_mask is not None:
            attn_mask = attn_mask.to(self.device)

        # Initialize KV cache
        with torch.inference_mode():
            outputs = self.model(
                input_ids=input_ids,
                attention_mask=attn_mask,
                use_cache=True,
            )

        return DecodeState(
            past_kv=outputs.past_key_values,
            logits=outputs.logits[
                :, -1, :
            ],  # Full vocab logits (shaped as batch, seq_len, vocab)
            generated=input_ids[0].tolist(),  # Maintain full history
        )
