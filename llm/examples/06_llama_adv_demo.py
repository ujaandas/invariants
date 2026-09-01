import json
import time

from llama_cpp import Llama


def main():
    print("Loading SOTA Grammar-Constrained LLM...")
    start_init = time.perf_counter()
    llm = Llama.from_pretrained(
        repo_id="Qwen/Qwen2.5-3B-Instruct-GGUF",
        filename="*q4_k_m.gguf",
        n_gpu_layers=-1,
        n_ctx=2048,
        verbose=False,
    )
    print(f"Engine ready in {time.perf_counter() - start_init:.2f}s")

    # Native JSON Schema dict for llama.cpp's Grammar Engine
    json_schema = {
        "type": "object",
        "properties": {
            "node_count": {"type": "number"},
            "profile": {
                "type": "object",
                "properties": {
                    "vcpu_cores": {"type": "number"},
                    "ram_gb": {"type": "number"},
                },
                "required": ["vcpu_cores", "ram_gb"],
            },
            "storage": {
                "type": "object",
                "properties": {
                    "storage_tb": {"type": "number"},
                    "redundancy": {"type": "string"},
                },
                "required": ["storage_tb", "redundancy"],
            },
            "compute_cost": {"type": "number"},
            "storage_cost": {"type": "number"},
            "total_monthly_bill": {"type": "number"},
        },
        "required": [
            "node_count",
            "profile",
            "storage",
            "compute_cost",
            "storage_cost",
            "total_monthly_bill",
        ],
    }

    prompt = (
        "You are an automated provisioning orchestrator. "
        "Calculate a quote for a cluster. "
        "node_count must be between 2 and 16. "
        "vcpu_cores between 2 and 64. ram_gb must be at least vcpu_cores * 2. "
        "storage_tb between 1 and 100. redundancy MUST be 'Geo-Redundant'. "
        "compute_cost is (vcpu_cores * 15.0 + ram_gb * 4.0) * node_count. "
        "storage_cost is storage_tb * 65.0. total_monthly_bill is compute_cost + storage_cost."
    )

    print("\n\033[1mSending prompt to LLM (SOTA Grammar Baseline)...\033[0m")
    t0 = time.perf_counter()

    # The response_format parameter forces llama.cpp to build a CFG and constrain the output
    response = llm.create_chat_completion(
        messages=[
            {"role": "system", "content": "You are a precise JSON API client."},
            {"role": "user", "content": prompt},
        ],
        response_format={"type": "json_object", "schema": json_schema},
        temperature=0.7,
        max_tokens=400,
    )

    elapsed_time = time.perf_counter() - t0
    output_text = response["choices"][0]["message"]["content"]
    tokens_generated = response["usage"]["completion_tokens"]

    print("\n\033[1m--- SOTA Grammar LLM Output ---\033[0m")
    print(output_text)

    print("\n\033[1m--- Semantic & Mathematical Validation Analysis ---\033[0m")
    try:
        # This will pass 100% of the time because of the JSON Schema enforcer
        data = json.loads(output_text.strip())
        print("\033[92m[PASS]\033[0m JSON Structural Syntax (Guaranteed by CFG)")

        def get_field(path, default=0.0):
            parts = path.split(".")
            cur = data
            for part in parts:
                if isinstance(cur, dict) and part in cur:
                    cur = cur[part]
                else:
                    return default
            return cur

        node_count = float(get_field("node_count", 0.0))
        vcpu_cores = float(get_field("profile.vcpu_cores", 0.0))
        ram_gb = float(get_field("profile.ram_gb", 0.0))
        storage_tb = float(get_field("storage.storage_tb", 0.0))
        redundancy = str(get_field("storage.redundancy", ""))

        compute_cost = float(get_field("compute_cost", 0.0))
        storage_cost = float(get_field("storage_cost", 0.0))
        total_monthly_bill = float(get_field("total_monthly_bill", 0.0))

        # 1. Invariant: Node Count
        if 2.0 <= node_count <= 16.0:
            print(
                f"\033[92m[PASS]\033[0m Invariant `cluster_scale`: node_count={node_count}"
            )
        else:
            print(
                f"\033[91m[FAIL]\033[0m Invariant `cluster_scale`: node_count={node_count}"
            )

        # 2. Invariant: Hardware Balance (vCPU & RAM)
        if 2.0 <= vcpu_cores <= 64.0:
            print(
                f"\033[92m[PASS]\033[0m Invariant `node_hardware_balance`: vcpu_cores={vcpu_cores}"
            )
        else:
            print(
                f"\033[91m[FAIL]\033[0m Invariant `node_hardware_balance`: vcpu_cores={vcpu_cores}"
            )

        if (ram_gb >= vcpu_cores * 2.0) and (ram_gb <= 256.0):
            print(
                f"\033[92m[PASS]\033[0m Invariant `node_hardware_balance`: ram_gb={ram_gb}"
            )
        else:
            print(
                f"\033[91m[FAIL]\033[0m Invariant `node_hardware_balance`: ram_gb={ram_gb} (Expected >= {vcpu_cores * 2.0})"
            )

        # 3. Invariant: Storage Limits & String Match
        if 1.0 <= storage_tb <= 100.0:
            print(
                f"\033[92m[PASS]\033[0m Invariant `capacity_limits`: storage_tb={storage_tb}"
            )
        else:
            print(
                f"\033[91m[FAIL]\033[0m Invariant `capacity_limits`: storage_tb={storage_tb}"
            )

        if redundancy == "Geo-Redundant":
            print(f"\033[92m[PASS]\033[0m String Constraint: redundancy='{redundancy}'")
        else:
            print(f"\033[91m[FAIL]\033[0m String Constraint: redundancy='{redundancy}'")

        # 4. Math: compute_cost
        expected_compute = (vcpu_cores * 15.0 + ram_gb * 4.0) * node_count
        if abs(expected_compute - compute_cost) < 0.01:
            print(
                f"\033[92m[PASS]\033[0m Math `compute_cost`: {compute_cost} == {expected_compute}"
            )
        else:
            print(
                f"\033[91m[FAIL]\033[0m Math `compute_cost`: Expected {expected_compute}, Got {compute_cost}"
            )

        # 5. Math: storage_cost
        expected_storage = storage_tb * 65.0
        if abs(expected_storage - storage_cost) < 0.01:
            print(
                f"\033[92m[PASS]\033[0m Math `storage_cost`: {storage_cost} == {expected_storage}"
            )
        else:
            print(
                f"\033[91m[FAIL]\033[0m Math `storage_cost`: Expected {expected_storage}, Got {storage_cost}"
            )

        # 6. Math: total_monthly_bill
        expected_total = compute_cost + storage_cost
        if abs(expected_total - total_monthly_bill) < 0.01:
            print(
                f"\033[92m[PASS]\033[0m Math `total_monthly_bill`: {total_monthly_bill} == {expected_total}"
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
