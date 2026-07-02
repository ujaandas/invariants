from transformers import AutoTokenizer, AutoModelForCausalLM, BitsAndBytesConfig
import torch

device = "cuda"

tokenizer = AutoTokenizer.from_pretrained("Qwen/Qwen2.5-3B-Instruct")

# 4 bit quantization
bnb_config = BitsAndBytesConfig(
    load_in_4bit=True,
    bnb_4bit_quant_type="nf4",
    bnb_4bit_use_double_quant=True,
    bnb_4bit_compute_dtype=torch.float16,
)

model = AutoModelForCausalLM.from_pretrained(
    "Qwen/Qwen2.5-3B-Instruct",
    quantization_config=bnb_config,
    device_map="cuda",
)

model.eval()

messages = [
    {"role": "user", "content": "Who are you?"},
]

inputs = tokenizer.apply_chat_template(
    messages,
    add_generation_prompt=True,
    tokenize=True,
    return_tensors="pt",
)

inputs = {k: v.to(device) for k, v in inputs.items()}

# Store outputted tokens
generated = []

with torch.inference_mode():
    # Prefill
    outputs = model(
        **inputs,
        use_cache=True,
    )

    past = outputs.past_key_values

    # Next token from prompt
    next_token = outputs.logits[:, -1].argmax(dim=-1, keepdim=True)

    for _ in range(40):
        # EOS check (forced sync but unavoidable)
        if next_token[0, 0].item() == tokenizer.eos_token_id:
            break

        generated.append(next_token)

        # Only feed ONE token + cache
        outputs = model(
            input_ids=next_token,
            past_key_values=past,
            use_cache=True,
        )

        past = outputs.past_key_values

        # Greedy decode
        next_token = outputs.logits[:, -1].argmax(dim=-1, keepdim=True)

generated = torch.cat(generated, dim=1)
print(tokenizer.decode(generated[0], skip_special_tokens=True))
