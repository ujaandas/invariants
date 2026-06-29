from transformers import AutoTokenizer, AutoModelForCausalLM
import torch

device = "cuda"

tokenizer = AutoTokenizer.from_pretrained("Qwen/Qwen2.5-3B-Instruct")
model = AutoModelForCausalLM.from_pretrained(
    "Qwen/Qwen2.5-3B-Instruct",
    dtype=torch.float16,
).to(device)

model.eval()  # Use eval mode

messages = [
    {"role": "user", "content": "Who are you?"},
]

inputs = tokenizer.apply_chat_template(
    messages,
    add_generation_prompt=True,
    tokenize=True,
    return_dict=True,
    return_tensors="pt",
)

inputs = {k: v.to(device) for k, v in inputs.items()}

# Since we need to control the generation loop (e.g. token masking), we cannot use model.generate()
# outputs = model.generate(**inputs, max_new_tokens=40)

input_ids = inputs["input_ids"]
attention_mask = inputs["attention_mask"]

for _ in range(40):
    with torch.no_grad():
        outputs = model(
            input_ids=input_ids,
            attention_mask=attention_mask,
        )

    # Logits for the newest token
    logits = outputs.logits[:, -1, :]

    # Greedy decoding
    next_token = logits.argmax(dim=-1, keepdim=True)

    # Append
    input_ids = torch.cat([input_ids, next_token], dim=-1)
    attention_mask = torch.cat(
        [attention_mask, torch.ones_like(next_token)],
        dim=-1,
    )

    # Stop on EOS
    if next_token.item() == tokenizer.eos_token_id:
        break

generated = input_ids[0, inputs["input_ids"].shape[-1] :]

print(tokenizer.decode(generated, skip_special_tokens=True))
