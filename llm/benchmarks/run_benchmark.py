import argparse
import csv
import json
import shutil
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
RESULTS_ROOT = Path("benchmarks/results")
SYSTEM_ROLLUP_FILES = {
    "Baseline_CFG": "baseline_cfg.csv",
    "Plain_Prompt": "plain_prompt.csv",
    "Invariants": "invariants.csv",
}


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
    parser.add_argument(
        "-v",
        "--verbose",
        action="store_true",
        help="Stream each generated token live instead of just the final "
        "JSON per case. Useful for diagnosing a stuck field.",
    )
    parser.add_argument(
        "--invariants-only",
        action="store_true",
        help="Skip the Baseline_CFG and Plain_Prompt systems entirely (no "
        "baseline model load, no extra generation calls) and only run "
        "Invariants. For isolating mask-computation timing without paying "
        "for the other two systems. Writes to results/mask_timing/ instead "
        "of the normal per-level location -- does not touch latest/ or the "
        "by_system rollups, which need all three systems present.",
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


def _strip_markdown_fence(text: str) -> str:
    # Plain-prompt models often wrap JSON in a ```json fence despite being
    # told not to; strip it so we're grading JSON content, not formatting.
    stripped = text.strip()
    if stripped.startswith("```"):
        lines = stripped.splitlines()
        if lines and lines[0].startswith("```"):
            lines = lines[1:]
        if lines and lines[-1].strip() == "```":
            lines = lines[:-1]
        stripped = "\n".join(lines).strip()
    return stripped


def crash_assertions(case: BenchmarkCase, error: Exception) -> list[dict]:
    # A crashed system produced no output at all -- every assertion the case
    # would have checked must count as failed, not be silently dropped from
    # the tally. An empty list here would let field-level aggregation treat
    # a crash as "this case just didn't contribute any data" instead of
    # "every field in this case failed," which is what actually happened.
    return [
        {
            "type": a.type,
            "field": a.field,
            "passed": False,
            "detail": f"system crashed before producing output: {error}",
        }
        for a in case.eval_assertions
    ]


def run_evaluations(case: BenchmarkCase, output_text: str) -> tuple[bool, list[dict]]:
    print(output_text)

    try:
        data = json.loads(_strip_markdown_fence(output_text))
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
        if assertion.field is not None:
            print(f"    {status} {assertion.type} on '{assertion.field}': {msg}")
        else:
            # "math" assertions span multiple fields via `expr` rather than
            # naming one -- `msg` already includes the expression, so don't
            # print a misleading "on 'None'".
            print(f"    {status} {assertion.type}: {msg}")
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


def run_baseline_case(llm: Llama, case: BenchmarkCase, temperature: float = 0.7) -> dict:
    t0 = time.perf_counter()
    response = llm.create_chat_completion(
        messages=[
            {"role": "system", "content": case.prompts.system},
            {"role": "user", "content": case.prompts.user},
        ],
        response_format={"type": "json_object", "schema": case.json_schema},
        temperature=temperature,
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


def run_plain_prompt_case(llm: Llama, case: BenchmarkCase, temperature: float = 0.7) -> dict:
    # Same prompt as run_baseline_case, but with no grammar constraint at all.
    t0 = time.perf_counter()
    response = llm.create_chat_completion(
        messages=[
            {"role": "system", "content": case.prompts.system},
            {"role": "user", "content": case.prompts.user},
        ],
        temperature=temperature,
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


def run_invariants_case(
    generator: ConstrainedGenerator, case: BenchmarkCase, verbose: bool = False,
    temperature: float = 0.0,
) -> dict:
    prompt_str = f"{case.prompts.system}\n\n{case.prompts.user}"
    result = generator.generate(
        case.invariants_dsl, case.root_spec, prompt_str, verbose=verbose,
        temperature=temperature,
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
        "total_fields": result.total_fields,
        "wall_time_s": wall_time,
        "tokens_per_sec": tps,
        "mask_time_s": result.mask_time_seconds,
        "mask_calls": result.mask_calls,
    }


def update_latest_pointer(out_dir: Path, level_dir: Path) -> None:
    # Copies this run's output into <level_dir>/latest/ so there's always one
    # unambiguous "current result" location.
    latest_dir = level_dir / "latest"
    latest_dir.mkdir(parents=True, exist_ok=True)
    for name in ("run.log", "results.csv", "results.json"):
        src = out_dir / name
        if src.exists():
            shutil.copyfile(src, latest_dir / name)


def rebuild_rollups(results_root: Path = RESULTS_ROOT) -> None:
    # Regenerates by_system/*.csv and all_results.csv from every level's
    # latest/ run, tagged with a Suite column.
    by_system: dict[str, list[list[str]]] = {name: [] for name in SYSTEM_ROLLUP_FILES}
    all_rows: list[list[str]] = []
    header: list[str] | None = None

    for level_dir in sorted(results_root.glob("schemas_*")):
        latest_csv = level_dir / "latest" / "results.csv"
        if not latest_csv.exists():
            continue
        with open(latest_csv, newline="", encoding="utf-8") as f:
            rows = list(csv.reader(f))
        if not rows:
            continue
        if header is None:
            header = ["Suite", *rows[0]]
        for row in rows[1:]:
            tagged = [level_dir.name, *row]
            all_rows.append(tagged)
            system = row[1] if len(row) > 1 else None
            if system in by_system:
                by_system[system].append(tagged)

    if header is None:
        return

    by_system_dir = results_root / "by_system"
    by_system_dir.mkdir(parents=True, exist_ok=True)
    for system_name, filename in SYSTEM_ROLLUP_FILES.items():
        with open(by_system_dir / filename, "w", newline="", encoding="utf-8") as f:
            w = csv.writer(f)
            w.writerow(header)
            w.writerows(by_system[system_name])

    with open(results_root / "all_results.csv", "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(header)
        w.writerows(all_rows)


def main():
    args = parse_args()
    suite_path = resolve_suite_path(args.suite)
    suite = BenchmarkSuite.load_from_file(str(suite_path))

    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")  # noqa: DTZ005
    level_dir = RESULTS_ROOT / suite_path.stem
    if args.invariants_only:
        # Separate tree: this run is missing Baseline_CFG/Plain_Prompt rows
        # entirely, so it must never become a level's latest/ or feed the
        # by_system rollups, which assume all three systems are present.
        out_dir = RESULTS_ROOT / "mask_timing" / suite_path.stem / timestamp
    else:
        out_dir = level_dir / timestamp
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
        llm = None
        if not args.invariants_only:
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
                    "Total_Fields",
                    "Wall_Time_s",
                    "Tokens_Per_Sec",
                    "Mask_Time_s",
                    "Mask_Calls",
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

                baseline = None
                plain_prompt = None
                if not args.invariants_only:
                    print("\n>>> Running Baseline (SOTA CFG / JSON Schema)")
                    try:
                        baseline = run_baseline_case(llm, case)
                    except Exception as e:  # noqa: BLE001
                        print(f"    Baseline generation crashed: {e}")
                        traceback.print_exc()
                        baseline = {
                            "raw_output": None,
                            "success": False,
                            "assertions": crash_assertions(case, e),
                            "tokens": 0,
                            "wall_time_s": 0.0,
                            "tokens_per_sec": 0.0,
                            "error": str(e),
                        }
                    case_payload["baseline"] = baseline

                    print("\n>>> Running Plain Prompt (no grammar)")
                    try:
                        plain_prompt = run_plain_prompt_case(llm, case)
                    except Exception as e:  # noqa: BLE001
                        print(f"    Plain prompt generation crashed: {e}")
                        traceback.print_exc()
                        plain_prompt = {
                            "raw_output": None,
                            "success": False,
                            "assertions": crash_assertions(case, e),
                            "tokens": 0,
                            "wall_time_s": 0.0,
                            "tokens_per_sec": 0.0,
                            "error": str(e),
                        }
                    case_payload["plain_prompt"] = plain_prompt

                print("\n>>> Running Invariants Architecture")
                try:
                    invariants = run_invariants_case(
                        generator, case, verbose=args.verbose
                    )
                except Exception as e:  # noqa: BLE001
                    print(f"    Invariants generation crashed: {e}")
                    traceback.print_exc()
                    invariants = {
                        "raw_output": None,
                        "success": False,
                        "assertions": crash_assertions(case, e),
                        "tokens": 0,
                        "fields_bypassed": 0,
                        "total_fields": 0,
                        "wall_time_s": 0.0,
                        "tokens_per_sec": 0.0,
                        "mask_time_s": 0.0,
                        "mask_calls": 0,
                        "error": str(e),
                    }
                case_payload["invariants"] = invariants

                run_payload["cases"].append(case_payload)
                # Persist after every case so a crash later in the suite
                # never loses data already gathered.
                flush_json()

                print(f"\n--- Summary for {case_id} ---")

                if baseline is not None:
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
                            0,
                            f"{baseline['wall_time_s']:.3f}",
                            f"{baseline['tokens_per_sec']:.2f}",
                            "0.000",
                            0,
                        ]
                    )
                    print(
                        f"Baseline   | Success: {baseline['success']} | "
                        f"Speed: {baseline['tokens_per_sec']:.2f} t/s | "
                        f"Tokens: {baseline['tokens']}"
                    )

                if plain_prompt is not None:
                    plain_passed = sum(1 for a in plain_prompt["assertions"] if a["passed"])
                    plain_total = len(plain_prompt["assertions"])
                    csv_writer.writerow(
                        [
                            case_id,
                            "Plain_Prompt",
                            plain_prompt["success"],
                            plain_passed,
                            plain_total,
                            plain_prompt["tokens"],
                            0,
                            0,
                            f"{plain_prompt['wall_time_s']:.3f}",
                            f"{plain_prompt['tokens_per_sec']:.2f}",
                            "0.000",
                            0,
                        ]
                    )
                    print(
                        f"Plain      | Success: {plain_prompt['success']} | "
                        f"Speed: {plain_prompt['tokens_per_sec']:.2f} t/s | "
                        f"Tokens: {plain_prompt['tokens']}"
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
                        invariants.get("total_fields", 0),
                        f"{invariants['wall_time_s']:.3f}",
                        f"{invariants['tokens_per_sec']:.2f}",
                        f"{invariants.get('mask_time_s', 0.0):.4f}",
                        invariants.get("mask_calls", 0),
                    ]
                )
                csv_file.flush()

                print(
                    f"Invariants | Success: {invariants['success']} | "
                    f"Speed: {invariants['tokens_per_sec']:.2f} t/s | "
                    f"Tokens: {invariants['tokens']} "
                    f"({invariants.get('fields_bypassed', 0)} fields bypassed) | "
                    f"Mask time: {invariants.get('mask_time_s', 0.0):.4f}s over "
                    f"{invariants.get('mask_calls', 0)} calls"
                )

        print(f"\nBenchmarks complete for suite '{suite_path.stem}'.")
        print(f"  Transcript: {log_path}")
        print(f"  CSV:        {csv_path}")
        print(f"  JSON:       {json_path}")
        if not args.invariants_only:
            update_latest_pointer(out_dir, level_dir)
            rebuild_rollups()
            print(f"  Latest:     {level_dir / 'latest'}")
            print(f"  Rollups:    {RESULTS_ROOT / 'by_system'} , {RESULTS_ROOT / 'all_results.csv'}")
        else:
            print("  (--invariants-only: latest/ pointer and rollups untouched)")
    finally:
        sys.stdout = real_stdout
        log_file.close()


if __name__ == "__main__":
    main()
