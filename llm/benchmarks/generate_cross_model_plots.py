"""Cross-model comparison charts and tables: Qwen2.5-3B-Instruct vs Gemma-2-2B-it."""

import json
from pathlib import Path

import matplotlib.pyplot as plt
import pandas as pd
from matplotlib.patches import Patch

from generate_plots import (
    SUITE_LABELS,
    SUITES,
    SYSTEM_COLORS,
    SYSTEM_LABELS,
    SYSTEM_ORDER,
    TEXT_PRIMARY,
    TEXT_SECONDARY,
    grouped_bars,
    print_values,
    style_axes,
)

QWEN_ROOT = Path("benchmarks/results")
GEMMA_ROOT = Path("benchmarks/results/cross_model/gemma-2-2b-it-gguf")
PLOTS_DIR = Path("benchmarks/plots/cross_model")
TABLES_PATH = Path("benchmarks/CROSS_MODEL_TABLES.md")

COLOR_QWEN = "#2a78d6"    # slot 1
COLOR_GEMMA = "#eb6834"   # slot 2

MODEL_LABELS = {"qwen": "Qwen2.5-3B-Instruct", "gemma": "Gemma-2-2B-it"}

_markdown_tables: list[str] = []


def save(fig, name):
    PLOTS_DIR.mkdir(parents=True, exist_ok=True)
    path = PLOTS_DIR / name
    fig.savefig(path, dpi=200, bbox_inches="tight", facecolor="white")
    plt.close(fig)
    print(f"  wrote {path}")


def record_table(title: str, header: list[str], rows: list[list[str]]):
    """Prints a table and appends it as markdown for CROSS_MODEL_TABLES.md."""
    print(f"\n  [table] {title}")
    widths = [max(len(str(h)), *(len(str(r[i])) for r in rows)) for i, h in enumerate(header)]
    print("    " + " | ".join(h.ljust(w) for h, w in zip(header, widths)))
    for r in rows:
        print("    " + " | ".join(str(c).ljust(w) for c, w in zip(r, widths)))

    md = [f"### {title}", "", "| " + " | ".join(header) + " |",
          "|" + "|".join(["---"] * len(header)) + "|"]
    for r in rows:
        md.append("| " + " | ".join(str(c) for c in r) + " |")
    md.append("")
    _markdown_tables.append("\n".join(md))


def load_suite_json(root: Path) -> dict:
    data = {}
    for suite in SUITES:
        p = root / suite / "latest" / "results.json"
        if p.exists():
            with open(p, encoding="utf-8") as f:
                data[suite] = json.load(f)
    return data


def load_all_results(root: Path) -> pd.DataFrame:
    p = root / "all_results.csv"
    df = pd.read_csv(p)
    df["Semantic_Success"] = df["Semantic_Success"].astype(bool)
    return df


# Chart CM-1: field-level pass rate by suite, Invariants only, both models

def field_level_by_suite(suite_json: dict, system: str = "Invariants") -> dict:
    key = {"Invariants": "invariants", "Baseline_CFG": "baseline"}[system]
    out = {}
    for suite in SUITES:
        if suite not in suite_json:
            continue
        passed = total = 0
        for case in suite_json[suite]["cases"]:
            r = case.get(key) or {}
            assertions = r.get("assertions") or []
            passed += sum(1 for a in assertions if a["passed"])
            total += len(assertions)
        out[suite] = (passed, total)
    return out


def plot_and_table_field_level_both_models(qwen_json: dict, gemma_json: dict):
    qwen_rates = field_level_by_suite(qwen_json)
    gemma_rates = field_level_by_suite(gemma_json)

    labels = [SUITE_LABELS[s] for s in SUITES if s in qwen_rates and s in gemma_rates]
    qwen_pct = [100 * qwen_rates[s][0] / qwen_rates[s][1] for s in SUITES if s in qwen_rates and s in gemma_rates]
    gemma_pct = [100 * gemma_rates[s][0] / gemma_rates[s][1] for s in SUITES if s in qwen_rates and s in gemma_rates]

    rows = []
    for s in SUITES:
        if s not in qwen_rates or s not in gemma_rates:
            continue
        qp, qt = qwen_rates[s]
        gp, gt = gemma_rates[s]
        match = "identical" if (qp, qt) == (gp, gt) else "differs"
        rows.append([SUITE_LABELS[s], f"{qp}/{qt} ({100*qp/qt:.1f}%)", f"{gp}/{gt} ({100*gp/gt:.1f}%)", match])
    record_table(
        "Field-level pass rate by suite, Invariants only -- Qwen vs Gemma",
        ["Suite", "Qwen2.5-3B", "Gemma-2-2B-it", "Match"],
        rows,
    )

    fig, ax = plt.subplots(figsize=(15, 5))
    grouped_bars(
        ax, labels,
        {"Qwen": qwen_pct, "Gemma": gemma_pct},
        {"Qwen": COLOR_QWEN, "Gemma": COLOR_GEMMA},
        {"Qwen": "Qwen2.5-3B-Instruct", "Gemma": "Gemma-2-2B-it"},
    )
    ax.set_ylabel("Individual assertions passed (%)")
    ax.set_ylim(0, 108)
    ax.set_title("Invariants field-level correctness, by suite -- two model families\n(BPE tokenizer vs SentencePiece; identical bars = identical numerator/denominator)",
                 fontsize=12, fontweight="bold")
    ax.legend(frameon=False, loc="upper left", bbox_to_anchor=(1.01, 1.0))
    style_axes(ax)
    save(fig, "cm_01_field_level_by_suite_both_models.png")


# Chart CM-2: dead-end tightness outcomes, both models

DEADEND_TIGHTNESS = {
    "DED_tightness_10pct": 0.10, "DED_tightness_30pct": 0.31,
    "DED_tightness_50pct": 0.52, "DED_tightness_70pct": 0.73,
    "DED_tightness_85pct": 0.89, "DED_tightness_92pct": 0.96,
}


def deadend_outcomes(suite_json: dict) -> dict:
    payload = suite_json.get("schemas_deadend_stress")
    if not payload:
        return {}
    out = {}
    for case in payload["cases"]:
        inv = case.get("invariants") or {}
        out[case["id"]] = {
            "crashed": inv.get("raw_output") is None,
            "raw_output": inv.get("raw_output"),
        }
    return out


def plot_and_table_deadend_both_models(qwen_json: dict, gemma_json: dict):
    qwen_out = deadend_outcomes(qwen_json)
    gemma_out = deadend_outcomes(gemma_json)

    ordered = sorted(DEADEND_TIGHTNESS.items(), key=lambda kv: kv[1])
    case_ids = [cid for cid, _ in ordered] + ["DED_sibling_dependent"]
    labels = [f"{DEADEND_TIGHTNESS[c]:.2f}" for c, _ in ordered] + ["sibling-\ndependent"]

    rows = []
    for cid, label in zip(case_ids, labels):
        qc = qwen_out.get(cid, {})
        gc = gemma_out.get(cid, {})
        q_status = "CRASHED" if qc.get("crashed") else "completed"
        g_status = "CRASHED" if gc.get("crashed") else "completed"
        match = "identical" if q_status == g_status else "DIFFERS"
        rows.append([label.replace("\n", " "), q_status, g_status, match])
    record_table(
        "No-backtrack dead-end outcome by tightness -- Qwen vs Gemma",
        ["Tightness ratio", "Qwen2.5-3B", "Gemma-2-2B-it", "Match"],
        rows,
    )

    default_rows = []
    for cid, _ in ordered[:-1]:  # exclude the 92pct case, it crashes
        qval = qwen_out.get(cid, {}).get("raw_output")
        gval = gemma_out.get(cid, {}).get("raw_output")
        default_rows.append([cid.replace("DED_tightness_", "").replace("pct", "%"),
                             repr(qval), repr(gval)])
    record_table(
        "Raw output per tightness level (fixed thresholds only) -- both models converge on the same value",
        ["Tightness", "Qwen output", "Gemma output"],
        default_rows,
    )

    outcomes_q = [0 if qwen_out.get(c, {}).get("crashed") else 1 for c in case_ids]
    outcomes_g = [0 if gemma_out.get(c, {}).get("crashed") else 1 for c in case_ids]

    fig, ax = plt.subplots(figsize=(11, 5))
    x = range(len(labels))
    width = 0.35
    for i, (name, outcomes, color) in enumerate(
        [("Qwen", outcomes_q, COLOR_QWEN), ("Gemma", outcomes_g, COLOR_GEMMA)]
    ):
        offset = (i - 0.5) * width
        xs = [xi + offset for xi in x]
        ax.bar(xs, [1] * len(outcomes), width, color=color, alpha=0.25, zorder=2)
        bar_colors = ["#1baf7a" if o == 1 else "#e34948" for o in outcomes]
        ax.bar(xs, [0.9] * len(outcomes), width, color=bar_colors, zorder=3)
        for xi, o in zip(xs, outcomes):
            ax.text(xi, 0.45, "OK" if o == 1 else "CRASH", ha="center", va="center",
                    fontsize=7.5, fontweight="bold", color="white", rotation=90)
    ax.set_xticks(list(x))
    ax.set_xticklabels(labels)
    ax.set_yticks([])
    ax.set_xlabel("Window-start / upper-bound ratio (higher = tighter)")
    ax.set_title("No-backtrack dead-end vs. tightness, Qwen vs Gemma\n(both models: crash only at 0.96 and the sibling-dependent case -- and share the same greedy default, 85)",
                 fontsize=11.5, fontweight="bold")
    style_axes(ax, y_grid=False)
    save(fig, "cm_02_deadend_tightness_both_models.png")


# Chart CM-3: free-text mask overhead, both models

FREETEXT_FIELD = {
    "FT_product_listing": "description",
    "FT_support_ticket": "body",
    "FT_article_summary": "summary",
}


def overhead(suite_json):
    payload = suite_json.get("schemas_long_freetext")
    out = {}
    if not payload:
        return out
    for case in payload["cases"]:
        inv = case.get("invariants") or {}
        mt = inv.get("mask_time_s", 0.0) or 0.0
        wt = inv.get("wall_time_s", 0.0) or 0.0
        raw = inv.get("raw_output")
        field_val = ""
        if raw:
            try:
                field_val = json.loads(raw).get(FREETEXT_FIELD[case["id"]], "")
            except (json.JSONDecodeError, KeyError):
                field_val = ""
        out[case["id"]] = {
            "mask_s": mt, "wall_s": wt,
            "pct": 100 * mt / wt if wt else 0.0,
            "field_chars": len(field_val),
            "degenerate": len(field_val.strip()) <= 2,
        }
    return out


def plot_and_table_freetext_overhead_both_models(qwen_json: dict, gemma_json: dict):
    qwen_oh = overhead(qwen_json)
    gemma_oh = overhead(gemma_json)
    case_ids = [c for c in qwen_oh if c in gemma_oh]

    rows = []
    for c in case_ids:
        q, g = qwen_oh[c], gemma_oh[c]
        g_content = f"DEGENERATE ({g['field_chars']} chars)" if g["degenerate"] else f"{g['field_chars']} chars, normal"
        rows.append([
            c,
            f"{q['pct']:.1f}% ({q['mask_s']:.1f}s/{q['wall_s']:.1f}s), {q['field_chars']} chars",
            f"{g['pct']:.1f}% ({g['mask_s']:.1f}s/{g['wall_s']:.1f}s), {g_content}",
        ])
    record_table(
        "Mask time as share of wall time, long free-text suite -- Qwen vs Gemma "
        "(NOTE: 2 of 3 Gemma cases produced degenerate single-character field content -- "
        "see the bug note below; only FT_support_ticket is a like-for-like overhead comparison)",
        ["Case", "Qwen2.5-3B", "Gemma-2-2B-it"],
        rows,
    )

    # Only the non-degenerate Gemma case is a fair overhead comparison
    valid_ids = [c for c in case_ids if not gemma_oh[c]["degenerate"]]
    degenerate_ids = [c for c in case_ids if gemma_oh[c]["degenerate"]]

    fig, ax = plt.subplots(figsize=(9, 5))
    x = range(len(case_ids))
    width = 0.35
    q_pcts = [qwen_oh[c]["pct"] for c in case_ids]
    g_pcts = [gemma_oh[c]["pct"] for c in case_ids]
    g_hatches = ["//" if c in degenerate_ids else None for c in case_ids]
    ax.bar([xi - width / 2 for xi in x], q_pcts, width, label=MODEL_LABELS["qwen"], color=COLOR_QWEN, zorder=3)
    for xi, v, c, hatch in zip(x, g_pcts, case_ids, g_hatches):
        ax.bar(xi + width / 2, v, width, color=COLOR_GEMMA, zorder=3, hatch=hatch,
               edgecolor="white" if hatch else COLOR_GEMMA, linewidth=1 if hatch else 0)
    for xi, v in zip(x, q_pcts):
        ax.text(xi - width / 2, v, f"{v:.0f}%", ha="center", va="bottom", fontsize=8, color=TEXT_SECONDARY)
    for xi, v in zip(x, g_pcts):
        ax.text(xi + width / 2, v, f"{v:.0f}%", ha="center", va="bottom", fontsize=8, color=TEXT_SECONDARY)
    ax.set_xticks(list(x))
    ax.set_xticklabels(case_ids, rotation=15, ha="right")
    ax.set_ylabel("Mask time / wall time (%)")
    ax.set_title("Mask overhead on the free-text worst case\nhatched bars are a Gemma bug (degenerate 1-char field), not a real timing result -- see FT_support_ticket for the fair comparison",
                 fontsize=11, fontweight="bold")
    legend_handles = [
        Patch(color=COLOR_QWEN, label=MODEL_LABELS["qwen"]),
        Patch(color=COLOR_GEMMA, label=MODEL_LABELS["gemma"]),
        Patch(facecolor=COLOR_GEMMA, hatch="//", edgecolor="white",
              label="Gemma, degenerate output (bug, not a timing result)"),
    ]
    ax.legend(handles=legend_handles, frameon=False, loc="upper left", bbox_to_anchor=(1.01, 1.0))
    style_axes(ax)
    save(fig, "cm_03_freetext_overhead_both_models.png")

    if degenerate_ids:
        bug_rows = []
        for c in degenerate_ids:
            g = gemma_oh[c]
            bug_rows.append([c, FREETEXT_FIELD[c], f"{g['field_chars']} char(s)", "no length/content assertion exists on unconstrained free-text fields (by design), so this passes silently"])
        record_table(
            "Gemma-specific bug: degenerate free-text output (undiagnosed, documented not fixed)",
            ["Case", "Field", "Actual content length", "Why it wasn't caught"],
            bug_rows,
        )


# Chart CM-4: field-level pass rate by suite, all 5 systems, meaned across both models

def plot_averaged_field_level(qwen_json: dict, gemma_json: dict):
    labels = []
    series = {sys_: [] for sys_ in SYSTEM_ORDER}
    raw = {}
    json_key = {
        "Plain_Prompt": "plain_prompt", "Baseline_CFG": "baseline",
        "Invariants": "invariants", "Outlines": "outlines", "Guidance": "guidance",
    }
    for suite in SUITES:
        if suite not in qwen_json or suite not in gemma_json:
            continue
        labels.append(SUITE_LABELS[suite])
        raw[SUITE_LABELS[suite]] = {}
        for sys_ in SYSTEM_ORDER:
            key = json_key[sys_]
            rates = []
            for suite_json in (qwen_json, gemma_json):
                passed = total = 0
                for case in suite_json[suite]["cases"]:
                    r = case.get(key) or {}
                    assertions = r.get("assertions") or []
                    passed += sum(1 for a in assertions if a["passed"])
                    total += len(assertions)
                if total:
                    rates.append(100 * passed / total)
            mean_rate = sum(rates) / len(rates) if rates else 0.0
            series[sys_].append(mean_rate)
            raw[SUITE_LABELS[suite]][sys_] = f"{mean_rate:.1f}% (n={len(rates)} models)"

    print_values("Field-level pass rate by suite, MEANED across Qwen + Gemma", raw)

    fig, ax = plt.subplots(figsize=(15, 5))
    grouped_bars(ax, labels, series, SYSTEM_COLORS, SYSTEM_LABELS)
    ax.set_ylabel("Individual assertions passed (%, meaned across 2 models)")
    ax.set_ylim(0, 108)
    ax.set_title("Field-level correctness by suite, averaged across Qwen2.5-3B and Gemma-2-2B-it",
                 fontsize=13, fontweight="bold")
    ax.legend(frameon=False, loc="upper left", bbox_to_anchor=(1.01, 1.0))
    style_axes(ax)
    save(fig, "cm_04_averaged_field_level_by_suite.png")


def main():
    print("Loading Qwen (primary) and Gemma (cross-model) datasets...")
    qwen_json = load_suite_json(QWEN_ROOT)
    gemma_json = load_suite_json(GEMMA_ROOT)
    print(f"  Qwen suites loaded:  {len(qwen_json)}/{len(SUITES)}")
    print(f"  Gemma suites loaded: {len(gemma_json)}/{len(SUITES)}")
    missing = set(SUITES) - set(gemma_json)
    if missing:
        print(f"  WARNING missing Gemma suites: {sorted(missing)}")

    print("\nGenerating cross-model charts and tables...")
    plot_and_table_field_level_both_models(qwen_json, gemma_json)
    plot_and_table_deadend_both_models(qwen_json, gemma_json)
    plot_and_table_freetext_overhead_both_models(qwen_json, gemma_json)
    plot_averaged_field_level(qwen_json, gemma_json)

    TABLES_PATH.write_text(
        "# Cross-Model Comparison Tables (Qwen2.5-3B-Instruct vs Gemma-2-2B-it)\n\n"
        "Auto-generated by generate_cross_model_plots.py. Pull these directly into the\n"
        "dissertation rather than retyping -- they're derived straight from\n"
        "benchmarks/results/ and benchmarks/results/cross_model/gemma-2-2b-it-gguf/.\n\n"
        + "\n".join(_markdown_tables)
    )
    print(f"\nWrote {TABLES_PATH}")
    print(f"Done. Cross-model charts written to {PLOTS_DIR}/")


if __name__ == "__main__":
    main()
