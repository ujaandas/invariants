import json

from llama_cpp import Llama


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
  /logistics/invoice:
    post:
      summary: Create a new logistics invoice
      requestBody:
        required: true
        content:
          application/json:
            schema:
              type: object
              required: [cargo_name, weight_kg, price_per_kg, base_cost, import_tariff, total_cost]
              properties:
                cargo_name:
                  type: string
                  example: "Industrial Titanium"
                weight_kg:
                  type: number
                  description: "Must be greater than 0.0 and less than or equal to 5000.0"
                price_per_kg:
                  type: number
                base_cost:
                  type: number
                  description: "The calculated base cost (weight_kg * price_per_kg)"
                import_tariff:
                  type: number
                  description: "The calculated import tariff (base_cost * 0.15)"
                total_cost:
                  type: number
                  description: "The calculated final total (base_cost + import_tariff)"
"""

    prompt = f"""You are an automated API client.
Construct a JSON payload to send a POST request to the endpoint defined below.
Pick random but realistic numbers for the shipment.

OpenAPI specification:
{openapi_spec}

Output only the raw JSON body for the request. No markdown or explanations."""

    print("\nSending prompt to LLM (Unconstrained)...")

    response = llm.create_chat_completion(
        messages=[
            {
                "role": "system",
                "content": "You are an API client that only outputs raw JSON payloads.",
            },
            {"role": "user", "content": prompt},
        ],
        temperature=0.7,
        max_tokens=250,
    )

    output_text = response["choices"][0]["message"]["content"]

    print("\n--- Unconstrained LLM Output ---")
    print(output_text)

    print("\n--- Analysis ---")

    # Strip markdown code blocks just in case the LLM ignores the system prompt
    clean_text = output_text.strip()
    clean_text = clean_text.removeprefix("```json")
    clean_text = clean_text.removeprefix("```")
    clean_text = clean_text.removesuffix("```")

    try:
        parsed = json.loads(clean_text.strip())
        print("[PASS] Valid JSON syntax")

        w = parsed.get("weight_kg", 0)
        p = parsed.get("price_per_kg", 0)
        bc = parsed.get("base_cost", 0)
        it = parsed.get("import_tariff", 0)
        tc = parsed.get("total_cost", 0)

        # Check Limits
        if 0.0 < w <= 5000.0:
            print(f"[PASS] Invariant: weight_kg ({w}) is within limits.")
        else:
            print(f"[FAIL] Invariant: weight_kg is {w} (Must be 0 < weight <= 5000).")

        # Check Math: base_cost
        expected_bc = w * p
        if abs(expected_bc - bc) < 0.001:
            print("[PASS] Math: base_cost is correct.")
        else:
            print(f"[FAIL] Math: base_cost (Expected {expected_bc}, Got {bc})")

        # Check Math: import_tariff
        expected_it = bc * 0.15
        if abs(expected_it - it) < 0.001:
            print("[PASS] Math: import_tariff is correct.")
        else:
            print(f"[FAIL] Math: import_tariff (Expected {expected_it}, Got {it})")

        # Check Math: total_cost
        expected_tc = bc + it
        if abs(expected_tc - tc) < 0.001:
            print("[PASS] Math: total_cost is correct.")
        else:
            print(f"[FAIL] Math: total_cost (Expected {expected_tc}, Got {tc})")

    except json.JSONDecodeError:
        print("[FAIL] Invalid JSON syntax")


if __name__ == "__main__":
    main()
