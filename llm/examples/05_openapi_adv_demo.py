import json
import time

from llama_cpp import Llama


def main():
    print("Loading unconstrained LLM...")
    start_init = time.perf_counter()
    llm = Llama.from_pretrained(
        repo_id="Qwen/Qwen2.5-3B-Instruct-GGUF",
        filename="*q4_k_m.gguf",
        n_gpu_layers=-1,
        n_ctx=2048,
        verbose=False,
    )
    print(f"Engine ready in {time.perf_counter() - start_init:.2f}s")

    # OpenAPI 3.0 representation matching the multi-spec CloudClusterQuote schema
    openapi_spec = """
paths:
  /cloud/cluster-quote:
    post:
      summary: Generate an enterprise Kubernetes cluster quote
      requestBody:
        required: true
        content:
          application/json:
            schema:
              $ref: '#/components/schemas/CloudClusterQuote'

components:
  schemas:
    NodeProfile:
      type: object
      required: [vcpu_cores, ram_gb]
      properties:
        vcpu_cores:
          type: number
          description: "Between 2.0 and 64.0 cores"
          example: 8.0
        ram_gb:
          type: number
          description: "Must be at least (vcpu_cores * 2.0) and at most 256.0 GB"
          example: 32.0

    StorageProfile:
      type: object
      required: [storage_tb, redundancy]
      properties:
        storage_tb:
          type: number
          description: "Between 1.0 and 100.0 TB"
          example: 10.0
        redundancy:
          type: string
          enum: ["Geo-Redundant"]
          description: "Must exactly match 'Geo-Redundant'"

    CloudClusterQuote:
      type: object
      required:
        - node_count
        - profile
        - storage
        - compute_cost
        - storage_cost
        - total_monthly_bill
      properties:
        node_count:
          type: number
          description: "Between 2.0 and 16.0 nodes"
          example: 4.0
        profile:
          $ref: '#/components/schemas/NodeProfile'
        storage:
          $ref: '#/components/schemas/StorageProfile'
        compute_cost:
          type: number
          description: "Calculated: (profile.vcpu_cores * 15.0 + profile.ram_gb * 4.0) * node_count"
        storage_cost:
          type: number
          description: "Calculated: storage.storage_tb * 65.0"
        total_monthly_bill:
          type: number
          description: "Calculated: compute_cost + storage_cost"
"""

    prompt = f"""You are an automated provisioning orchestrator.
Construct a JSON payload to send a POST request to the endpoint defined below.
Pick random but realistic numbers and calculate all dependent cost fields accurately.

OpenAPI specification:
{openapi_spec}

Output ONLY raw JSON. Do not include markdown code blocks, backticks, or any conversational text."""

    print("\n\033[1mSending prompt to LLM (Unconstrained Baseline)...\033[0m")
    t0 = time.perf_counter()

    response = llm.create_chat_completion(
        messages=[
            {
                "role": "system",
                "content": "You are an API client that only outputs raw JSON payloads.",
            },
            {"role": "user", "content": prompt},
        ],
        temperature=0.7,
        max_tokens=400,
    )

    elapsed_time = time.perf_counter() - t0
    output_text = response["choices"][0]["message"]["content"]
    tokens_generated = response["usage"]["completion_tokens"]

    print("\n\033[1m--- Unconstrained LLM Output ---\033[0m")
    print(output_text)

    # Clean markdown artifacts if generated
    clean_text = output_text.strip()
    clean_text = clean_text.removeprefix("```json")
    clean_text = clean_text.removeprefix("```")
    clean_text = clean_text.removesuffix("```")

    print("\n\033[1m--- Semantic & Mathematical Validation Analysis ---\033[0m")
    try:
        data = json.loads(clean_text.strip())
        print("[PASS] JSON Structural Syntax")

        # Extract fields supporting both nested JSON and flattened dot-notation
        def get_field(path, default=0.0):
            parts = path.split(".")
            cur = data
            for part in parts:
                if isinstance(cur, dict) and part in cur:
                    cur = cur[part]
                else:
                    return default
            return cur

        node_count = float(
            get_field("node_count", get_field("CloudClusterQuote.node_count", 0.0))
        )
        vcpu_cores = float(
            get_field("profile.vcpu_cores", get_field("vcpu_cores", 0.0))
        )
        ram_gb = float(get_field("profile.ram_gb", get_field("ram_gb", 0.0)))
        storage_tb = float(
            get_field("storage.storage_tb", get_field("storage_tb", 0.0))
        )
        redundancy = str(get_field("storage.redundancy", get_field("redundancy", "")))

        compute_cost = float(get_field("compute_cost", 0.0))
        storage_cost = float(get_field("storage_cost", 0.0))
        total_monthly_bill = float(get_field("total_monthly_bill", 0.0))

        # 1. Invariant: Node Count
        if 2.0 <= node_count <= 16.0:
            print(
                f"[PASS] Invariant `cluster_scale`: node_count={node_count} (in [2.0, 16.0])"
            )
        else:
            print(
                f"\033[91m[FAIL]\033[0m Invariant `cluster_scale`: node_count={node_count} (Out of bounds [2.0, 16.0])"
            )

        # 2. Invariant: Hardware Balance (vCPU & RAM)
        if 2.0 <= vcpu_cores <= 64.0:
            print(
                f"[PASS] Invariant `node_hardware_balance`: vcpu_cores={vcpu_cores} (in [2.0, 64.0])"
            )
        else:
            print(
                f"\033[91m[FAIL]\033[0m Invariant `node_hardware_balance`: vcpu_cores={vcpu_cores} (Out of bounds [2.0, 64.0])"
            )

        if (ram_gb >= vcpu_cores * 2.0) and (ram_gb <= 256.0):
            print(
                f"[PASS] Invariant `node_hardware_balance`: ram_gb={ram_gb} (>= vcpu*2={vcpu_cores * 2.0} and <= 256.0)"
            )
        else:
            print(
                f"\033[91m[FAIL]\033[0m Invariant `node_hardware_balance`: ram_gb={ram_gb} (Expected >= {vcpu_cores * 2.0} and <= 256.0)"
            )

        # 3. Invariant: Storage Limits & String Match
        if 1.0 <= storage_tb <= 100.0:
            print(
                f"[PASS] Invariant `capacity_limits`: storage_tb={storage_tb} (in [1.0, 100.0])"
            )
        else:
            print(
                f"\033[91m[FAIL]\033[0m Invariant `capacity_limits`: storage_tb={storage_tb} (Out of bounds [1.0, 100.0])"
            )

        if redundancy == "Geo-Redundant":
            print(f"[PASS] String Constraint: redundancy='{redundancy}'")
        else:
            print(
                f"\033[91m[FAIL]\033[0m String Constraint: redundancy='{redundancy}' (Expected 'Geo-Redundant')"
            )

        # 4. Math: compute_cost = (vcpu * 15 + ram * 4) * node_count
        expected_compute = (vcpu_cores * 15.0 + ram_gb * 4.0) * node_count
        if abs(expected_compute - compute_cost) < 0.01:
            print(f"[PASS] Math `compute_cost`: {compute_cost} == {expected_compute}")
        else:
            print(
                f"\033[91m[FAIL]\033[0m Math `compute_cost`: Expected {expected_compute}, Got {compute_cost}"
            )

        # 5. Math: storage_cost = storage_tb * 65.0
        expected_storage = storage_tb * 65.0
        if abs(expected_storage - storage_cost) < 0.01:
            print(f"[PASS] Math `storage_cost`: {storage_cost} == {expected_storage}")
        else:
            print(
                f"\033[91m[FAIL]\033[0m Math `storage_cost`: Expected {expected_storage}, Got {storage_cost}"
            )

        # 6. Math: total_monthly_bill = compute_cost + storage_cost
        expected_total = compute_cost + storage_cost
        if abs(expected_total - total_monthly_bill) < 0.01:
            print(
                f"[PASS] Math `total_monthly_bill`: {total_monthly_bill} == {expected_total}"
            )
        else:
            print(
                f"\033[91m[FAIL]\033[0m Math `total_monthly_bill`: Expected {expected_total}, Got {total_monthly_bill}"
            )

    except json.JSONDecodeError:
        print(
            "\033[91m[FAIL]\033[0m JSON Structural Syntax: Failed to parse LLM response into JSON."
        )

    print("\n\033[1m--- Benchmark Metrics ---\033[0m")
    print(f"LLM Tokens Sampled:          {tokens_generated}")
    print(f"Total Wall Time:             {elapsed_time:.3f}s")
    if elapsed_time > 0:
        print(
            f"Sampling Speed:              {tokens_generated / elapsed_time:.2f} tok/s"
        )


if __name__ == "__main__":
    main()
