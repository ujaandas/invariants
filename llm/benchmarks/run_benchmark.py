import csv
import json
import time

from helper import BenchmarkCase, BenchmarkSuite
from invariants.Engine import Engine
from invariants.Processor import ConstrainedGenerator
from llama_cpp import Llama


def run_evaluations(case: BenchmarkCase, output_text: str) -> bool:
    print(output_text)
    try:
        data = json.loads(output_text.strip())
    except json.JSONDecodeError:
        print("    Invalid JSON syntax generated.")
        return False

    all_passed = True
    for assertion in case.eval_assertions:
        if assertion.evaluate(data):
            print(f"    Passed {assertion.type} on '{assertion.field}'")
        else:
            print(f"    Failed {assertion.type} on '{assertion.field}'")
            all_passed = False

    return all_passed


def main():
    # Initialize baseline
    llm = Llama.from_pretrained(
        repo_id="Qwen/Qwen2.5-3B-Instruct-GGUF",
        filename="*q4_k_m.gguf",
        n_gpu_layers=-1,
        n_ctx=2048,
        verbose=False,
    )

    # Initialize invariants
    engine = Engine()
    generator = ConstrainedGenerator(engine)

    # Load test suite
    suite_filename = "benchmarks/schemas_l1.json"
    suite = BenchmarkSuite.load_from_file(suite_filename)

    # Prepare CSV Writer
    with open(
        f"{suite_filename}_results.csv",
        mode="w",
        newline="",
        encoding="utf-8",
    ) as csv_file:
        csv_writer = csv.writer(csv_file)
        csv_writer.writerow(
            [
                "Benchmark_ID",
                "System",
                "Semantic_Success",
                "Tokens_Generated",
                "Fields_Bypassed",
                "Wall_Time_s",
                "Tokens_Per_Sec",
            ]
        )

        for case_id, case in suite.benchmarks.items():
            print("\n==================================================")
            print(f"Evaluating: {case.name}")
            print(f"Domain: {case.domain}")
            print("==================================================")

            prompt_str = f"{case.prompts.system}\n\n{case.prompts.user}"

            # Run baseline
            print("\n>>> Running Baseline (SOTA CFG / JSON Schema)")
            t0 = time.perf_counter()
            response = llm.create_chat_completion(
                messages=[
                    {"role": "system", "content": case.prompts.system},
                    {"role": "user", "content": case.prompts.user},
                ],
                response_format={
                    "type": "json_object",
                    "schema": case.json_schema,
                },
                temperature=0.7,
                max_tokens=400,
            )
            base_time = time.perf_counter() - t0
            base_text = response["choices"][0]["message"]["content"]
            base_tokens = response["usage"]["completion_tokens"]
            base_tps = base_tokens / base_time if base_time > 0 else 0

            base_success = run_evaluations(case, base_text)

            csv_writer.writerow(
                [
                    case_id,
                    "Baseline_CFG",
                    base_success,
                    base_tokens,
                    0,
                    f"{base_time:.3f}",
                    f"{base_tps:.2f}",
                ]
            )

            # Run invariants
            print("\n>>> Running Invariants Architecture")

            inv_result = generator.generate(
                case.invariants_dsl,
                case.root_spec,
                prompt_str,
                verbose=False,
            )

            inv_text = inv_result.json_output
            inv_time = inv_result.wall_time_seconds
            inv_tokens = inv_result.tokens_sampled
            inv_bypassed = getattr(inv_result, "fields_bypassed", 0)
            inv_tps = inv_tokens / inv_time if inv_time > 0 else 0

            inv_success = run_evaluations(case, inv_text)

            csv_writer.writerow(
                [
                    case_id,
                    "Invariants",
                    inv_success,
                    inv_tokens,
                    inv_bypassed,
                    f"{inv_time:.3f}",
                    f"{inv_tps:.2f}",
                ]
            )

            print(f"\n--- Summary for {case_id} ---")
            print(
                f"Baseline   | Success: {base_success} | "
                f"Speed: {base_tps:.2f} t/s | Tokens: {base_tokens}"
            )
            print(
                f"Invariants | Success: {inv_success} | "
                f"Speed: {inv_tps:.2f} t/s | Tokens: {inv_tokens} "
                f"({inv_bypassed} fields bypassed)"
            )

    print("\nBenchmarks complete! Data written to benchmark_results.csv")


if __name__ == "__main__":
    main()
