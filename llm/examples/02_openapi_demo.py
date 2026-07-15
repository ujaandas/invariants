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

    openapi_spec = """
paths:
  /orders/bulk:
    post:
      summary: Create a new bulk order
      requestBody:
        required: true
        content:
          application/json:
            schema:
              type: object
              required: [unit_price, quantity, currency, total_price]
              properties:
                unit_price:
                  type: number
                  example: 15.50
                quantity:
                  type: integer
                  example: 100
                currency:
                  type: string
                  example: "USD"
                total_price:
                  type: number
                  description: The calculated total (unit_price * quantity)
"""

    prompt = f"""You are an automated API client.
Construct a JSON payload to send a POST request to the endpoint defined below.
Pick random but realistic numbers.

OpenAPI specification:
{openapi_spec}

Output only the raw JSON body for the request. No markdown or explanations."""

    print("\nSending prompt to LLM")

    response = llm.create_chat_completion(
        messages=[
            {
                "role": "system",
                "content": "You are an API client that only outputs raw JSON payloads.",
            },
            {"role": "user", "content": prompt},
        ],
        temperature=1.0,
        max_tokens=200,
    )

    output_text = response["choices"][0]["message"]["content"]

    print("\nUnconstrained LLM output")
    print(output_text)

    print("\nAnalysis")
    try:
        parsed = json.loads(output_text)
        print("Valid JSON syntax")

        expected_total = parsed.get("unit_price", 0) * parsed.get("quantity", 0)
        actual_total = parsed.get("total_price", 0)

        if expected_total == actual_total:
            print("Math is correct!")
        else:
            print(
                f"Math failed! Expected {expected_total}, but LLM wrote {actual_total}"
            )

    except json.JSONDecodeError:
        print("Invalid JSON syntax")


if __name__ == "__main__":
    main()
