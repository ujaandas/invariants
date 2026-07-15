import mlx.core as mx
from mlx_lm import load
from mlx_lm.generate import generate_step

model, tokenizer = load("mlx-community/Qwen2.5-3B-Instruct-4bit")

messages = [{"role": "user", "content": "Who are you?"}]
prompt = tokenizer.apply_chat_template(
    messages, tokenize=False, add_generation_prompt=True
)
prompt_tokens = mx.array(tokenizer.encode(prompt))


def constrained_sampler(logits: mx.array) -> mx.array:
    return mx.argmax(logits, axis=-1)


generated = []

for token, _ in generate_step(prompt_tokens, model, sampler=constrained_sampler):
    if token == tokenizer.eos_token_id or len(generated) >= 40:
        break

    generated.append(token)

print(tokenizer.decode(generated))
