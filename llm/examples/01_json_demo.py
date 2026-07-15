from llama_cpp import Llama
import json


def main():
    print("Loading unconstrained LLM...")
    llm = Llama.from_pretrained(
        repo_id="Qwen/Qwen2.5-3B-Instruct-GGUF",
        filename="*q4_k_m.gguf",
        n_gpu_layers=-1,
        verbose=False,
    )

    schema = """
    {
    "unit_price": Number,
    "quantity": Number,
    "currency": String,
    "total_price": Number
    }
    """

    prompt = f"""You are a data generation assistant. 
Generate a JSON object representing a bulk order.
Use exactly this structure and fill in realistic values:
{schema}

Do not include any explanations, greetings, or markdown formatting (like ```json). Output ONLY the raw JSON object."""
    print("\nSending prompt to LLM")

    # Using the standard, unconstrained chat completion API
    response = llm.create_chat_completion(
        messages=[
            {"role": "system", "content": "You output strict JSON without markdown."},
            {"role": "user", "content": prompt},
        ],
        temperature=1.0,
        max_tokens=200,
    )

    output_text = response["choices"][0]["message"]["content"]

    print("\nUnconstrained LLM output")
    print(output_text)

    # See if it actually output valid JSON that we can parse
    print("\nAnalysis")
    try:
        parsed = json.loads(output_text)
        print("Valid JSON syntax")

        # Check global math constraint
        expected_total = parsed.get("unit_price", 0) * parsed.get("quantity", 0)
        actual_total = parsed.get("total_price", 0)

        if expected_total == actual_total:
            print("Math is correct!")
        else:
            print(
                f"Math Failed! Expected {expected_total}, but LLM wrote {actual_total}"
            )

    except json.JSONDecodeError:
        print("Invalid JSON syntax")


if __name__ == "__main__":
    main()
