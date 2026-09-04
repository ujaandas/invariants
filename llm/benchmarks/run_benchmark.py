import argparse
import csv
import json
import sys
import time
import traceback
from datetime import datetime
from pathlib import Path

from helper import BenchmarkCase, BenchmarkSuite
from invariants.Engine import Engine, resolve_cached_model_path
from invariants.Processor import ConstrainedGenerator
from llama_cpp import Llama

REPO_ID = "Qwen/Qwen2.5-3B-Instruct-GGUF"
MODEL_FILENAME = "*q4_k_m.gguf"


class Tee:
    def __init__(self, *streams):
        self.streams = streams

    def write(self, data):
        for s in self.streams:
            s.write(data)

    def flush(self):
        for s in self.streams:
            s.flush()


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Run an invariants benchmark suite (baseline CFG vs. invariants) "
        "and persist a full transcript + structured results for later analysis."
    )
    parser.add_argument(
        "suite",
        nargs="?",
        default="l1",
        help="Suite to run: a level shorthand (l1, l2, l3, l4) or an explicit "
        "path to a schemas_*.json file. Default: l1",
    )
    return parser.parse_args()


def resolve_suite_path(suite_arg: str) -> Path:
    candidate = Path(suite_arg)
    if candidate.exists():
        return candidate

    normalized = suite_arg.lower().lstrip("l")
    shorthand = Path("benchmarks") / f"schemas_l{normalized}.json"
    if shorthand.exists():
        return shorthand

    raise FileNotFoundError(
        f"Could not resolve benchmark suite '{suite_arg}' (tried '{candidate}' "
        f"and '{shorthand}')"
    )


def run_evaluations(case: BenchmarkCase, output_text: str) -> tuple[bool, list[dict]]:
    print(output_text)

    try:
        data = json.loads(output_text.strip())
    except json.JSONDecodeError as e:
        print(f"    Invalid JSON syntax generated: {e}")
        return False, [
            {
                "type": "json_parse",
                "field": None,
                "passed": False,
                "detail": str(e),
            }
        ]

    all_passed = True
    assertions_detail = []
    for assertion in case.eval_assertions:
        passed, msg = assertion.evaluate_detailed(data)
        status = "Passed" if passed else "Failed"
        print(f"    {status} {assertion.type} on '{assertion.field}': {msg}")
        assertions_detail.append(
            {
                "type": assertion.type,
                "field": assertion.field,
                "passed": passed,
                "detail": msg,
            }
        )
        if not passed:
            all_passed = False

    return all_passed, assertions_detail


def load_baseline_llm() -> Llama:
    model_path = resolve_cached_model_path(REPO_ID, MODEL_FILENAME)
    if model_path is not None:
        return Llama(
            model_path=model_path,
            n_gpu_layers=-1,
            n_ctx=2048,
            verbose=False,
        )
    # Not cached yet: fall back to the network-aware loader, which downloads
    # and populates the cache for next time.
    return Llama.from_pretrained(
        repo_id=REPO_ID,
        filename=MODEL_FILENAME,
        n_gpu_layers=-1,
        n_ctx=2048,
        verbose=False,
    )


def run_baseline_case(llm: Llama, case: BenchmarkCase) -> dict:
    t0 = time.perf_counter()
    response = llm.create_chat_completion(
        messages=[
            {"role": "system", "content": case.prompts.system},
            {"role": "user", "content": case.prompts.user},
        ],
        response_format={"type": "json_object", "schema": case.json_schema},
        temperature=0.7,
        max_tokens=400,
    )
    wall_time = time.perf_counter() - t0
    text = response["choices"][0]["message"]["content"]
    tokens = response["usage"]["completion_tokens"]
    tps = tokens / wall_time if wall_time > 0 else 0

    success, assertions = run_evaluations(case, text)
    return {
        "raw_output": text,
        "success": success,
        "assertions": assertions,
        "tokens": tokens,
        "wall_time_s": wall_time,
        "tokens_per_sec": tps,
    }


def run_invariants_case(generator: ConstrainedGenerator, case: BenchmarkCase) -> dict:
    prompt_str = f"{case.prompts.system}\n\n{case.prompts.user}"
    result = generator.generate(
        case.invariants_dsl, case.root_spec, prompt_str, verbose=False
    )

    tokens = result.tokens_sampled
    wall_time = result.wall_time_seconds
    tps = tokens / wall_time if wall_time > 0 else 0

    success, assertions = run_evaluations(case, result.json_output)
    return {
        "raw_output": result.json_output,
        "success": success,
        "assertions": assertions,
        "tokens": tokens,
        "fields_bypassed": result.fields_bypassed,
        "wall_time_s": wall_time,
        "tokens_per_sec": tps,
    }


def main():
    args = parse_args()
    suite_path = resolve_suite_path(args.suite)
    suite = BenchmarkSuite.load_from_file(str(suite_path))

    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")  # noqa: DTZ005
    out_dir = Path("benchmarks/results") / suite_path.stem / timestamp
    out_dir.mkdir(parents=True, exist_ok=True)

    log_path = out_dir / "run.log"
    csv_path = out_dir / "results.csv"
    json_path = out_dir / "results.json"

    log_file = open(log_path, "w", encoding="utf-8")  # noqa: SIM115
    real_stdout = sys.stdout
    sys.stdout = Tee(real_stdout, log_file)

    run_payload = {
        "suite": suite_path.stem,
        "level_description": suite.level_description,
        "generated_at": timestamp,
        "model": {"repo_id": REPO_ID, "filename": MODEL_FILENAME},
        "cases": [],
    }

    def flush_json():
        with open(json_path, "w", encoding="utf-8") as f:
            json.dump(run_payload, f, indent=2)

    try:
        print("Initializing baseline (SOTA CFG / JSON Schema) model...")
        llm = load_baseline_llm()

        print("Initializing invariants engine...")
        engine = Engine()
        generator = ConstrainedGenerator(engine)

        with open(csv_path, mode="w", newline="", encoding="utf-8") as csv_file:
            csv_writer = csv.writer(csv_file)
            csv_writer.writerow(
                [
                    "Benchmark_ID",
                    "System",
                    "Semantic_Success",
                    "Assertions_Passed",
                    "Assertions_Total",
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

                case_payload = {
                    "id": case_id,
                    "name": case.name,
                    "domain": case.domain,
                }

                print("\n>>> Running Baseline (SOTA CFG / JSON Schema)")
                try:
                    baseline = run_baseline_case(llm, case)
                except Exception as e:  # noqa: BLE001
                    print(f"    Baseline generation crashed: {e}")
                    traceback.print_exc()
                    baseline = {
                        "raw_output": None,
                        "success": False,
                        "assertions": [],
                        "tokens": 0,
                        "wall_time_s": 0.0,
                        "tokens_per_sec": 0.0,
                        "error": str(e),
                    }
                case_payload["baseline"] = baseline

                print("\n>>> Running Invariants Architecture")
                try:
                    invariants = run_invariants_case(generator, case)
                except Exception as e:  # noqa: BLE001
                    print(f"    Invariants generation crashed: {e}")
                    traceback.print_exc()
                    invariants = {
                        "raw_output": None,
                        "success": False,
                        "assertions": [],
                        "tokens": 0,
                        "fields_bypassed": 0,
                        "wall_time_s": 0.0,
                        "tokens_per_sec": 0.0,
                        "error": str(e),
                    }
                case_payload["invariants"] = invariants

                run_payload["cases"].append(case_payload)
                # Persist after every case so a crash later in the suite
                # never loses data already gathered.
                flush_json()

                base_passed = sum(1 for a in baseline["assertions"] if a["passed"])
                base_total = len(baseline["assertions"])
                csv_writer.writerow(
                    [
                        case_id,
                        "Baseline_CFG",
                        baseline["success"],
                        base_passed,
                        base_total,
                        baseline["tokens"],
                        0,
                        f"{baseline['wall_time_s']:.3f}",
                        f"{baseline['tokens_per_sec']:.2f}",
                    ]
                )

                inv_passed = sum(1 for a in invariants["assertions"] if a["passed"])
                inv_total = len(invariants["assertions"])
                csv_writer.writerow(
                    [
                        case_id,
                        "Invariants",
                        invariants["success"],
                        inv_passed,
                        inv_total,
                        invariants["tokens"],
                        invariants.get("fields_bypassed", 0),
                        f"{invariants['wall_time_s']:.3f}",
                        f"{invariants['tokens_per_sec']:.2f}",
                    ]
                )
                csv_file.flush()

                print(f"\n--- Summary for {case_id} ---")
                print(
                    f"Baseline   | Success: {baseline['success']} | "
                    f"Speed: {baseline['tokens_per_sec']:.2f} t/s | "
                    f"Tokens: {baseline['tokens']}"
                )
                print(
                    f"Invariants | Success: {invariants['success']} | "
                    f"Speed: {invariants['tokens_per_sec']:.2f} t/s | "
                    f"Tokens: {invariants['tokens']} "
                    f"({invariants.get('fields_bypassed', 0)} fields bypassed)"
                )

        print(f"\nBenchmarks complete for suite '{suite_path.stem}'.")
        print(f"  Transcript: {log_path}")
        print(f"  CSV:        {csv_path}")
        print(f"  JSON:       {json_path}")
    finally:
        sys.stdout = real_stdout
        log_file.close()


if __name__ == "__main__":
    main()
