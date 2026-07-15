import numpy as np
from llama_cpp import Llama, LogitsProcessorList

llm = Llama.from_pretrained(
    repo_id="Qwen/Qwen2.5-3B-Instruct-GGUF",
    filename="*q4_k_m.gguf",  # 4-bit quantization
    n_gpu_layers=-1,
    verbose=False,
)

messages = [{"role": "user", "content": "Who are you?"}]
# prompt_str = llm.metadata_to_chat_formatter(messages)
# if not prompt_str:
prompt_str = "<|im_start|>user\nWho are you?<|im_end|>\n<|im_start|>assistant\n"

prompt_tokens = llm.tokenize(prompt_str.encode("utf-8"))


def constrained_logits_processor(
    input_ids: list[int], scores: np.ndarray
) -> np.ndarray:
    return scores


processors = LogitsProcessorList([constrained_logits_processor])

generated = []

for token in llm.generate(prompt_tokens, logits_processor=processors):
    if token == llm.token_eos() or len(generated) >= 40:
        break

    generated.append(token)

# Decode bytes back to string
print(llm.detokenize(generated).decode("utf-8"))
