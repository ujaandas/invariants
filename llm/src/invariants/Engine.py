from transformers import AutoTokenizer, AutoModelForCausalLM, BitsAndBytesConfig
import torch


class Engine:
    def __init__(
        self,
        model_name: str,
        tokenizer_name: str | None = None,
        device: str = "cuda",
        quantize: bool = True,
    ):
        self.device = device

        self.tokenizer = AutoTokenizer.from_pretrained(tokenizer_name or model_name)

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
