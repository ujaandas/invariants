import csv
import hashlib
import json
import sys
import time
import traceback
from datetime import datetime
from pathlib import Path

from helper import BenchmarkSuite
from invariants.Engine import Engine
from invariants.Processor import ConstrainedGenerator
from run_benchmark import (
    Tee,
    crash_assertions,
    load_baseline_llm,
    run_baseline_case,
    run_invariants_case,
    run_plain_prompt_case,
)

RESULTS_ROOT = Path("benchmarks/results/temperature_sweep")

# Six cases spanning bypass-heavy, zero-bypass, deep-dependency, and both new
# adversarial categories -- not the full ~26-case corpus, to keep the N=5 x
# 2-temperature x 3-system compute bill bounded while still answering:
# (a) is Invariants-at-temp=0 actually bit-identical across repeats,
# (b) does matching temperature change baseline's/plain's standing,
# (c) does temperature give Invariants a way out of a dead end.
SWEEP_CASES = [
    ("schemas_l1.json", "L1_user_onboarding"),
    ("schemas_l2.json", "L2_retail_order_billing"),
    ("schemas_schemastore.json", "SS_aws_autoscaling_group"),
    ("schemas_dependency_chains.json", "DEP_project_task_schedule"),
    ("schemas_deadend_stress.json", "DED_tightness_50pct"),
    ("schemas_long_freetext.json", "FT_support_ticket"),
]
TEMPERATURES = [0.0, 0.7]
N_REPEATS = 5


def output_hash(text: str | None) -> str:
    if text is None:
        return "CRASH"
    return hashlib.sha256(text.encode("utf-8")).hexdigest()[:16]


def load_cases() -> dict[str, object]:
    cases = {}
    suite_cache: dict[str, BenchmarkSuite] = {}
    for suite_file, case_id in SWEEP_CASES:
        if suite_file not in suite_cache:
            suite_cache[suite_file] = BenchmarkSuite.load_from_file(
                str(Path("benchmarks") / suite_file)
            )
        suite = suite_cache[suite_file]
        if case_id not in suite.benchmarks:
            raise KeyError(f"'{case_id}' not found in {suite_file}")
        cases[case_id] = (suite_file, suite.benchmarks[case_id])
    return cases


def main():
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")  # noqa: DTZ005
    out_dir = RESULTS_ROOT / timestamp
    out_dir.mkdir(parents=True, exist_ok=True)

    log_path = out_dir / "run.log"
    csv_path = out_dir / "results.csv"
    json_path = out_dir / "raw_outputs.json"

    log_file = open(log_path, "w", encoding="utf-8")  # noqa: SIM115
    real_stdout = sys.stdout
    sys.stdout = Tee(real_stdout, log_file)

    raw_outputs: list[dict] = []

    def flush_json():
        with open(json_path, "w", encoding="utf-8") as f:
            json.dump(raw_outputs, f, indent=2)

    try:
        cases = load_cases()

        print("Initializing baseline (SOTA CFG / JSON Schema) model...")
        llm = load_baseline_llm()
        print("Initializing invariants engine...")
        engine = Engine()
        generator = ConstrainedGenerator(engine)

        with open(csv_path, mode="w", newline="", encoding="utf-8") as csv_file:
            csv_writer = csv.writer(csv_file)
            csv_writer.writerow(
                [
                    "Suite",
                    "Benchmark_ID",
                    "System",
                    "Temperature",
                    "Trial",
                    "Semantic_Success",
                    "Assertions_Passed",
                    "Assertions_Total",
                    "Tokens",
                    "Wall_Time_s",
                    "Tokens_Per_Sec",
                    "Output_Hash",
                ]
            )

            for case_id, (suite_file, case) in cases.items():
                for temperature in TEMPERATURES:
                    invariants_hashes = []
                    for trial in range(1, N_REPEATS + 1):
                        print("\n==================================================")
                        print(f"{case_id} | temp={temperature} | trial {trial}/{N_REPEATS}")
                        print("==================================================")

                        print("\n>>> Baseline_CFG")
                        try:
                            baseline = run_baseline_case(llm, case, temperature=temperature)
                        except Exception as e:  # noqa: BLE001
                            print(f"    Baseline crashed: {e}")
                            traceback.print_exc()
                            baseline = {
                                "raw_output": None,
                                "success": False,
                                "assertions": crash_assertions(case, e),
                                "tokens": 0,
                                "wall_time_s": 0.0,
                                "tokens_per_sec": 0.0,
                            }

                        print("\n>>> Plain_Prompt")
                        try:
                            plain = run_plain_prompt_case(llm, case, temperature=temperature)
                        except Exception as e:  # noqa: BLE001
                            print(f"    Plain prompt crashed: {e}")
                            traceback.print_exc()
                            plain = {
                                "raw_output": None,
                                "success": False,
                                "assertions": crash_assertions(case, e),
                                "tokens": 0,
                                "wall_time_s": 0.0,
                                "tokens_per_sec": 0.0,
                            }

                        print("\n>>> Invariants")
                        try:
                            invariants = run_invariants_case(
                                generator, case, temperature=temperature
                            )
                        except Exception as e:  # noqa: BLE001
                            print(f"    Invariants crashed: {e}")
                            traceback.print_exc()
                            invariants = {
                                "raw_output": None,
                                "success": False,
                                "assertions": crash_assertions(case, e),
                                "tokens": 0,
                                "wall_time_s": 0.0,
                                "tokens_per_sec": 0.0,
                            }

                        for system_name, result in (
                            ("Baseline_CFG", baseline),
                            ("Plain_Prompt", plain),
                            ("Invariants", invariants),
                        ):
                            passed = sum(1 for a in result["assertions"] if a["passed"])
                            total = len(result["assertions"])
                            h = output_hash(result["raw_output"])
                            csv_writer.writerow(
                                [
                                    suite_file,
                                    case_id,
                                    system_name,
                                    temperature,
                                    trial,
                                    result["success"],
                                    passed,
                                    total,
                                    result["tokens"],
                                    f"{result['wall_time_s']:.3f}",
                                    f"{result['tokens_per_sec']:.2f}",
                                    h,
                                ]
                            )
                            if system_name == "Invariants":
                                invariants_hashes.append(h)
                            raw_outputs.append(
                                {
                                    "case_id": case_id,
                                    "system": system_name,
                                    "temperature": temperature,
                                    "trial": trial,
                                    "raw_output": result["raw_output"],
                                    "hash": h,
                                }
                            )
                        csv_file.flush()
                        flush_json()

                        print(
                            f"\n--- {case_id} temp={temperature} trial={trial} ---\n"
                            f"Baseline   hash={output_hash(baseline['raw_output'])} "
                            f"success={baseline['success']}\n"
                            f"Plain      hash={output_hash(plain['raw_output'])} "
                            f"success={plain['success']}\n"
                            f"Invariants hash={output_hash(invariants['raw_output'])} "
                            f"success={invariants['success']}"
                        )

                    distinct = set(invariants_hashes)
                    print(
                        f"\n[determinism check] {case_id} temp={temperature}: "
                        f"{len(distinct)} distinct output(s) across {N_REPEATS} "
                        f"Invariants trials -- hashes={invariants_hashes}"
                    )

        print(f"\nTemperature sweep complete. CSV: {csv_path}  Raw outputs: {json_path}")
    finally:
        sys.stdout = real_stdout
        log_file.close()


if __name__ == "__main__":
    main()
