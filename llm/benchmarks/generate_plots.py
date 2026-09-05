"""
Generates comparison charts (Plain Prompt vs Baseline CFG vs Invariants) from
benchmark results and writes them as PNGs to benchmarks/plots/.

Reads two kinds of data:
  - benchmarks/results/all_results.csv               (case-level metrics, all 3 systems)
  - benchmarks/results/schemas_*/latest/results.json  (per-assertion detail, all 3 systems)
  - benchmarks/results/mask_timing/schemas_*/*/       (mask-computation timing + total_fields,
                                                        Invariants only, from --invariants-only runs)
"""

import json
from pathlib import Path

import matplotlib.pyplot as plt
import pandas as pd

RESULTS_ROOT = Path("benchmarks/results")
PLOTS_DIR = Path("benchmarks/plots")

# Categorical palette, fixed order (dataviz skill reference palette, slots 1-3 --
# these three validate together for both adjacent and all-pairs CVD checks).
COLOR_PLAIN = "#2a78d6"
COLOR_BASELINE = "#eb6834"
COLOR_INVARIANTS = "#1baf7a"
# Diverging pair, for the one polarity chart (faster/slower than baseline).
COLOR_DIV_FASTER = "#2a78d6"
COLOR_DIV_SLOWER = "#e34948"

TEXT_PRIMARY = "#0b0b0b"
TEXT_SECONDARY = "#52514e"
GRID_COLOR = "#e3e2dd"

SYSTEM_ORDER = ["Plain_Prompt", "Baseline_CFG", "Invariants"]
SYSTEM_COLORS = {
    "Plain_Prompt": COLOR_PLAIN,
    "Baseline_CFG": COLOR_BASELINE,
    "Invariants": COLOR_INVARIANTS,
}
SYSTEM_LABELS = {
    "Plain_Prompt": "Plain Prompt",
    "Baseline_CFG": "Baseline (GBNF)",
    "Invariants": "Invariants",
}
JSON_KEY_FOR_SYSTEM = {
    "Plain_Prompt": "plain_prompt",
    "Baseline_CFG": "baseline",
    "Invariants": "invariants",
}

SUITES = [
    "schemas_l1",
    "schemas_l2",
    "schemas_l3",
    "schemas_l4",
    "schemas_schemastore",
    "schemas_realworld2",
    "schemas_offload_showcase",
    "schemas_dependency_chains",
    "schemas_scheduling_stress",
    "schemas_deadend_stress",
    "schemas_long_freetext",
]
SUITE_LABELS = {
    "schemas_l1": "L1",
    "schemas_l2": "L2",
    "schemas_l3": "L3",
    "schemas_l4": "L4",
    "schemas_schemastore": "SchemaStore",
    "schemas_realworld2": "RealWorld2",
    "schemas_offload_showcase": "Offload",
    "schemas_dependency_chains": "DepChains",
    "schemas_scheduling_stress": "SchedFix",
    "schemas_deadend_stress": "DeadEnd*",
    "schemas_long_freetext": "FreeText*",
}
# Suites marked with * are deliberately adversarial stress tests, not
# representative-usage suites -- included in every per-suite chart so
# failures show up in context rather than being cropped out, but worth
# calling out in captions rather than silently averaged away.
ADVERSARIAL_SUITES = {"schemas_deadend_stress", "schemas_long_freetext"}

TEMP_SWEEP_ROOT = RESULTS_ROOT / "temperature_sweep"

plt.rcParams["font.family"] = "sans-serif"
plt.rcParams["font.size"] = 10
plt.rcParams["text.color"] = TEXT_PRIMARY


def style_axes(ax, y_grid=True):
    for side in ("top", "right", "left"):
        ax.spines[side].set_visible(False)
    ax.spines["bottom"].set_color(GRID_COLOR)
    if y_grid:
        ax.grid(axis="y", color=GRID_COLOR, linewidth=1, zorder=0)
    ax.set_axisbelow(True)
    ax.tick_params(colors=TEXT_SECONDARY, length=0)
    for lbl in ax.get_xticklabels() + ax.get_yticklabels():
        lbl.set_color(TEXT_SECONDARY)


def save(fig, name):
    PLOTS_DIR.mkdir(parents=True, exist_ok=True)
    path = PLOTS_DIR / name
    fig.savefig(path, dpi=200, bbox_inches="tight", facecolor="white")
    plt.close(fig)
    print(f"  wrote {path}")


def print_values(title: str, data):
    # Dumps the exact numbers a chart is about to render, so a wrong-looking
    # bar can be checked against the underlying aggregate directly instead of
    # reverse-engineered from the rendered PNG.
    print(f"  [values] {title}:")
    if isinstance(data, dict):
        for k, v in data.items():
            print(f"    {k}: {v}")
    else:
        print(f"    {data}")


def grouped_bars(ax, categories, series: dict, colors: dict, labels: dict,
                  value_fmt=None, min_visible_frac=0.006):
    """series: {system: [value per category]}. A value of exactly 0 renders
    as a thin stub (min_visible_frac of the axis range) rather than nothing
    at all -- a true zero and a missing bar must not look identical."""
    n = len(series)
    width = 0.8 / n
    x = range(len(categories))
    all_vals = [v for vals in series.values() for v in vals]
    axis_span = max(all_vals) if all_vals else 1
    stub = axis_span * min_visible_frac
    for i, (name, values) in enumerate(series.items()):
        offset = (i - (n - 1) / 2) * width
        xs = [xi + offset for xi in x]
        heights = [v if v > 0 else stub for v in values]
        bars = ax.bar(xs, heights, width, label=labels.get(name, name),
                      color=colors.get(name, "#888"), zorder=3)
        if value_fmt:
            for bx, v, h in zip(xs, values, heights):
                ax.text(bx, h, value_fmt(v), ha="center", va="bottom",
                        fontsize=7.5, color=TEXT_SECONDARY)
    ax.set_xticks(list(x))
    ax.set_xticklabels(categories)


# ---------------------------------------------------------------------------
# Data loading
# ---------------------------------------------------------------------------

def load_all_results() -> pd.DataFrame:
    df = pd.read_csv(RESULTS_ROOT / "all_results.csv")
    df["Semantic_Success"] = df["Semantic_Success"].astype(bool)
    return df


def load_suite_json() -> dict:
    data = {}
    for suite in SUITES:
        p = RESULTS_ROOT / suite / "latest" / "results.json"
        if p.exists():
            with open(p, encoding="utf-8") as f:
                data[suite] = json.load(f)
    return data


def load_mask_timing_json() -> dict:
    data = {}
    for suite in SUITES:
        base = RESULTS_ROOT / "mask_timing" / suite
        if not base.exists():
            continue
        runs = sorted(p for p in base.iterdir() if p.is_dir())
        if not runs:
            continue
        p = runs[-1] / "results.json"
        if p.exists():
            with open(p, encoding="utf-8") as f:
                data[suite] = json.load(f)
    return data


def load_temperature_sweep() -> pd.DataFrame | None:
    if not TEMP_SWEEP_ROOT.exists():
        return None
    runs = sorted(p for p in TEMP_SWEEP_ROOT.iterdir() if p.is_dir())
    if not runs:
        return None
    csv_path = runs[-1] / "results.csv"
    if not csv_path.exists():
        return None
    df = pd.read_csv(csv_path)
    df["Semantic_Success"] = df["Semantic_Success"].astype(bool)
    return df


# ---------------------------------------------------------------------------
# Chart 1: field-level (assertion) pass rate, by suite -- the primary
# correctness figure. Case-level "all or nothing" success is relegated to a
# secondary chart (01b) since it makes systems that get most fields right
# but miss one (e.g. Plain_Prompt) look like they failed "literally
# everything," which overstates the gap.
# ---------------------------------------------------------------------------

def plot_field_level_pass_rate_by_suite(suite_json: dict):
    labels = []
    series = {sys_: [] for sys_ in SYSTEM_ORDER}
    raw = {}
    for suite in SUITES:
        if suite not in suite_json:
            continue
        totals = {sys_: [0, 0] for sys_ in SYSTEM_ORDER}
        for case in suite_json[suite]["cases"]:
            for sys_ in SYSTEM_ORDER:
                key = JSON_KEY_FOR_SYSTEM[sys_]
                if key not in case:
                    continue
                assertions = case[key].get("assertions") or []
                totals[sys_][0] += sum(1 for a in assertions if a["passed"])
                totals[sys_][1] += len(assertions)
        labels.append(SUITE_LABELS[suite])
        raw[SUITE_LABELS[suite]] = {}
        for sys_ in SYSTEM_ORDER:
            p, t = totals[sys_]
            rate = 100 * p / t if t else 0
            series[sys_].append(rate)
            raw[SUITE_LABELS[suite]][sys_] = f"{p}/{t} ({rate:.1f}%)"

    print_values("Field-level pass rate by suite (passed/total, rate)", raw)

    fig, ax = plt.subplots(figsize=(13, 5))
    grouped_bars(ax, labels, series, SYSTEM_COLORS, SYSTEM_LABELS)
    ax.set_ylabel("Individual assertions passed (%)")
    ax.set_ylim(0, 108)
    ax.set_title("Field-level correctness, by suite\n(counts a crashed case as every field failed, not excluded)",
                 fontsize=13, fontweight="bold")
    ax.legend(frameon=False, loc="upper left", bbox_to_anchor=(1.01, 1.0))
    style_axes(ax)
    save(fig, "01_field_level_pass_rate_by_suite.png")


# ---------------------------------------------------------------------------
# Chart 1b: case-level ("all assertions in the case passed") success rate,
# by suite -- secondary/supplementary view of the same underlying data.
# ---------------------------------------------------------------------------

def plot_success_rate_by_suite(df: pd.DataFrame):
    labels = [SUITE_LABELS[s] for s in SUITES if s in df["Suite"].unique()]
    series = {sys_: [] for sys_ in SYSTEM_ORDER}
    raw = {}
    for suite in SUITES:
        sub = df[df["Suite"] == suite]
        if sub.empty:
            continue
        raw[SUITE_LABELS[suite]] = {}
        for sys_ in SYSTEM_ORDER:
            rows = sub[sub["System"] == sys_]
            rate = 100 * rows["Semantic_Success"].mean() if len(rows) else 0
            series[sys_].append(rate)
            raw[SUITE_LABELS[suite]][sys_] = f"{rate:.1f}% (n={len(rows)})"
    labels.append("All")
    raw["All"] = {}
    for sys_ in SYSTEM_ORDER:
        rows = df[df["System"] == sys_]
        rate = 100 * rows["Semantic_Success"].mean() if len(rows) else 0
        series[sys_].append(rate)
        raw["All"][sys_] = f"{rate:.1f}% (n={len(rows)})"

    print_values("Case-level (all-assertions-pass) success rate by suite", raw)

    fig, ax = plt.subplots(figsize=(13, 5))
    grouped_bars(ax, labels, series, SYSTEM_COLORS, SYSTEM_LABELS)
    ax.set_ylabel("Cases passed (%)")
    ax.set_ylim(0, 108)
    ax.set_title("Case-level success rate, by suite (secondary view -- one\nmissed field fails the whole case)",
                 fontsize=13, fontweight="bold")
    ax.legend(frameon=False, loc="upper left", bbox_to_anchor=(1.01, 1.0))
    ax.axvline(len(labels) - 1.5, color=GRID_COLOR, linewidth=1)
    style_axes(ax)
    save(fig, "01b_case_level_success_by_suite.png")


# ---------------------------------------------------------------------------
# Chart 2: assertion-level (field-level) pass rate, aggregated
# ---------------------------------------------------------------------------

def plot_assertion_pass_rate_overall(suite_json: dict):
    totals = {sys_: [0, 0] for sys_ in SYSTEM_ORDER}  # [passed, total]
    for suite, payload in suite_json.items():
        for case in payload["cases"]:
            for sys_ in SYSTEM_ORDER:
                key = JSON_KEY_FOR_SYSTEM[sys_]
                if key not in case:
                    continue
                assertions = case[key].get("assertions") or []
                totals[sys_][0] += sum(1 for a in assertions if a["passed"])
                totals[sys_][1] += len(assertions)

    labels = [SYSTEM_LABELS[s] for s in SYSTEM_ORDER]
    rates = [100 * totals[s][0] / totals[s][1] if totals[s][1] else 0 for s in SYSTEM_ORDER]
    colors = [SYSTEM_COLORS[s] for s in SYSTEM_ORDER]

    print_values("Assertion pass rate, aggregated overall (passed/total)",
                 {SYSTEM_LABELS[s]: f"{totals[s][0]}/{totals[s][1]} ({rates[i]:.1f}%)"
                  for i, s in enumerate(SYSTEM_ORDER)})

    fig, ax = plt.subplots(figsize=(6, 5))
    bars = ax.bar(labels, rates, width=0.55, color=colors, zorder=3)
    for b, s in zip(bars, SYSTEM_ORDER):
        p, t = totals[s]
        ax.text(b.get_x() + b.get_width() / 2, b.get_height(), f"{b.get_height():.0f}%\n({p}/{t})",
                ha="center", va="bottom", fontsize=9, color=TEXT_SECONDARY)
    ax.set_ylabel("Individual assertions passed (%)")
    ax.set_ylim(0, 112)
    ax.set_title("Field-level correctness, aggregated across\nall suites and cases",
                 fontsize=13, fontweight="bold")
    style_axes(ax)
    save(fig, "02_assertion_pass_rate_overall.png")


# ---------------------------------------------------------------------------
# Chart 3: assertion pass rate, by assertion type -- the key chart
# ---------------------------------------------------------------------------

def plot_assertion_pass_rate_by_type(suite_json: dict):
    # "json_parse" is special: run_evaluations only ever records ONE when
    # parsing FAILS (a successful parse just proceeds to check the schema's
    # real assertions and never emits a "json_parse passed" entry). Pulling
    # it from the assertions list the same way as the others would make it
    # tautologically 0% for every system. Compute it per-case instead: did
    # this case's output parse as JSON at all?
    types_order = ["json_parse", "range", "membership", "exact_value", "math"]
    type_labels = {
        "range": "Range", "membership": "Membership (enum)",
        "exact_value": "Exact value", "math": "Cross-field / math",
        "json_parse": "Produced valid JSON",
    }
    counts = {sys_: {t: [0, 0] for t in types_order} for sys_ in SYSTEM_ORDER}
    for suite, payload in suite_json.items():
        for case in payload["cases"]:
            for sys_ in SYSTEM_ORDER:
                key = JSON_KEY_FOR_SYSTEM[sys_]
                if key not in case:
                    continue
                assertions = case[key].get("assertions") or []
                counts[sys_]["json_parse"][1] += 1
                if not any(a["type"] == "json_parse" for a in assertions):
                    counts[sys_]["json_parse"][0] += 1
                for a in assertions:
                    t = a["type"]
                    if t not in counts[sys_] or t == "json_parse":
                        continue
                    counts[sys_][t][1] += 1
                    if a["passed"]:
                        counts[sys_][t][0] += 1

    present_types = [t for t in types_order if any(counts[s][t][1] for s in SYSTEM_ORDER)]
    labels = [type_labels[t] for t in present_types]
    series = {
        sys_: [100 * counts[sys_][t][0] / counts[sys_][t][1] if counts[sys_][t][1] else 0
               for t in present_types]
        for sys_ in SYSTEM_ORDER
    }

    print_values("Assertion pass rate by type (passed/total per system)", {
        type_labels[t]: {
            SYSTEM_LABELS[s]: f"{counts[s][t][0]}/{counts[s][t][1]}" for s in SYSTEM_ORDER
        }
        for t in present_types
    })

    fig, ax = plt.subplots(figsize=(11, 5.5))
    grouped_bars(ax, labels, series, SYSTEM_COLORS, SYSTEM_LABELS)
    ax.set_ylabel("Passed (%)")
    ax.set_ylim(0, 108)
    ax.set_title("Where each system succeeds and fails, by constraint type",
                 fontsize=13, fontweight="bold")
    ax.legend(frameon=False, loc="upper left", bbox_to_anchor=(1.01, 1.0))
    style_axes(ax)
    save(fig, "03_assertion_pass_rate_by_type.png")


# ---------------------------------------------------------------------------
# Chart 4: deterministic-bypass rate by suite (Invariants only) -- transparency
# ---------------------------------------------------------------------------

def plot_bypass_rate_by_suite(mask_json: dict):
    labels, rates = [], []
    raw = {}
    for suite in SUITES:
        if suite not in mask_json:
            continue
        bypassed = total = 0
        for case in mask_json[suite]["cases"]:
            inv = case.get("invariants") or {}
            bypassed += inv.get("fields_bypassed", 0) or 0
            total += inv.get("total_fields", 0) or 0
        if total == 0:
            continue
        labels.append(SUITE_LABELS[suite])
        rates.append(100 * bypassed / total)
        raw[SUITE_LABELS[suite]] = f"{bypassed}/{total} fields ({rates[-1]:.1f}%)"

    print_values("Deterministic-bypass rate by suite (fields bypassed / total fields -- a field COUNT ratio, not a time share)", raw)

    fig, ax = plt.subplots(figsize=(10, 5))
    bars = ax.bar(labels, rates, width=0.55, color=COLOR_INVARIANTS, zorder=3)
    for b in bars:
        ax.text(b.get_x() + b.get_width() / 2, b.get_height(), f"{b.get_height():.0f}%",
                ha="center", va="bottom", fontsize=9, color=TEXT_SECONDARY)
    ax.set_ylabel("Fields deterministically bypassed (% of field COUNT, not time)")
    ax.set_ylim(0, 100)
    ax.set_title("Share of fields skipped entirely by AOT solving, by suite\n(a fraction of how many fields never touch the LLM -- see chart 08 for the time-based view)",
                 fontsize=13, fontweight="bold")
    style_axes(ax)
    save(fig, "04_bypass_rate_by_suite.png")


# ---------------------------------------------------------------------------
# Chart 5 & 6: tokens generated / wall time, by system
# ---------------------------------------------------------------------------

def plot_mean_metric_by_suite(df: pd.DataFrame, column: str, ylabel: str, title: str, filename: str):
    labels = [SUITE_LABELS[s] for s in SUITES if s in df["Suite"].unique()]
    series = {sys_: [] for sys_ in SYSTEM_ORDER}
    raw = {}
    for suite in SUITES:
        sub = df[df["Suite"] == suite]
        if sub.empty:
            continue
        raw[SUITE_LABELS[suite]] = {}
        for sys_ in SYSTEM_ORDER:
            rows = sub[sub["System"] == sys_]
            val = rows[column].mean() if len(rows) else 0
            series[sys_].append(val)
            raw[SUITE_LABELS[suite]][sys_] = round(val, 3)
    labels.append("All")
    raw["All"] = {}
    for sys_ in SYSTEM_ORDER:
        rows = df[df["System"] == sys_]
        val = rows[column].mean() if len(rows) else 0
        series[sys_].append(val)
        raw["All"][sys_] = round(val, 3)

    print_values(f"Mean {column} by suite", raw)

    fig, ax = plt.subplots(figsize=(13, 5))
    grouped_bars(ax, labels, series, SYSTEM_COLORS, SYSTEM_LABELS)
    ax.set_ylabel(ylabel)
    ax.set_title(title, fontsize=13, fontweight="bold")
    ax.legend(frameon=False, loc="upper left", bbox_to_anchor=(1.01, 1.0))
    ax.axvline(len(labels) - 1.5, color=GRID_COLOR, linewidth=1)
    style_axes(ax)
    save(fig, filename)


# ---------------------------------------------------------------------------
# Chart 7: per-case wall-time speedup/slowdown vs. Baseline, as a percentage
# -- a ratio bar chart anchored at 1.0 reads as "how far above the line," a
# percentage reads directly as "how much faster/slower," so this is framed
# as %% change rather than a raw ratio.
# ---------------------------------------------------------------------------

def plot_wall_time_ratio(df: pd.DataFrame):
    inv = df[df["System"] == "Invariants"][["Suite", "Benchmark_ID", "Wall_Time_s"]]
    base = df[df["System"] == "Baseline_CFG"][["Suite", "Benchmark_ID", "Wall_Time_s"]]
    merged = inv.merge(base, on=["Suite", "Benchmark_ID"], suffixes=("_inv", "_base"))
    merged = merged[(merged["Wall_Time_s_base"] > 0) & (merged["Wall_Time_s_inv"] > 0)]
    # pct_change > 0 means Invariants took longer (slower); < 0 means faster.
    merged["pct_change"] = 100 * (merged["Wall_Time_s_inv"] / merged["Wall_Time_s_base"] - 1)
    merged = merged.sort_values("pct_change")

    labels = [f"{SUITE_LABELS.get(s, s)}: {b}" for s, b in zip(merged["Suite"], merged["Benchmark_ID"])]
    pct = merged["pct_change"].tolist()
    colors = [COLOR_DIV_FASTER if p < 0 else COLOR_DIV_SLOWER for p in pct]

    print_values("Wall-time %% change, Invariants vs. Baseline, per case (negative = Invariants faster)",
                 dict(zip(labels, [f"{p:+.1f}%" for p in pct])))
    median_pct = merged["pct_change"].median() if len(merged) else 0.0
    print(f"    median: {median_pct:+.1f}%")

    fig, ax = plt.subplots(figsize=(9, max(4, 0.32 * len(labels))))
    y = range(len(labels))
    ax.barh(y, pct, color=colors, zorder=3, height=0.6)
    ax.axvline(0.0, color=TEXT_SECONDARY, linewidth=1.2)
    ax.set_yticks(list(y))
    ax.set_yticklabels(labels, fontsize=8)
    ax.set_xlabel("Wall-time change vs. Baseline (%) -- negative = Invariants faster")
    ax.set_title(f"Wall-clock speedup/slowdown vs. baseline, per case\n(median: {median_pct:+.1f}%)",
                 fontsize=13, fontweight="bold")
    style_axes(ax, y_grid=False)
    ax.grid(axis="x", color=GRID_COLOR, linewidth=1, zorder=0)
    save(fig, "07_wall_time_pct_change_per_case.png")


# ---------------------------------------------------------------------------
# Chart 8: mask-computation time as a share of total Invariants wall time
# ---------------------------------------------------------------------------

def plot_mask_overhead_share(mask_json: dict):
    labels, mask_frac, other_frac, mask_abs = [], [], [], []
    raw = {}
    for suite in SUITES:
        if suite not in mask_json:
            continue
        mt = wt = 0.0
        for case in mask_json[suite]["cases"]:
            inv = case.get("invariants") or {}
            mt += inv.get("mask_time_s", 0.0) or 0.0
            wt += inv.get("wall_time_s", 0.0) or 0.0
        if wt <= 0:
            continue
        labels.append(SUITE_LABELS[suite])
        mask_abs.append(mt)
        mask_frac.append(100 * mt / wt)
        other_frac.append(100 * (1 - mt / wt))
        raw[SUITE_LABELS[suite]] = f"mask={mt:.3f}s / wall={wt:.3f}s ({mask_frac[-1]:.1f}%)"

    print_values("Mask time as share of total Invariants wall time, by suite", raw)

    fig, ax = plt.subplots(figsize=(10, 5))
    x = range(len(labels))
    ax.bar(x, other_frac, width=0.55, bottom=mask_frac, color="#d9d8d2",
           label="LLM inference & other", zorder=3)
    ax.bar(x, mask_frac, width=0.55, color=COLOR_INVARIANTS,
           label="Mask computation", zorder=3)
    for xi, mf, ma in zip(x, mask_frac, mask_abs):
        ax.text(xi, mf, f"{mf:.1f}%\n({ma:.2f}s)", ha="center", va="bottom",
                fontsize=8, color=TEXT_SECONDARY)
    ax.set_xticks(list(x))
    ax.set_xticklabels(labels)
    ax.set_ylabel("Share of Invariants wall time (%)")
    ax.set_ylim(0, 115)
    ax.set_title("Mask computation as a share of total generation time",
                 fontsize=13, fontweight="bold")
    ax.legend(frameon=False, loc="upper left", bbox_to_anchor=(1.01, 1.0))
    style_axes(ax)
    save(fig, "08_mask_overhead_share.png")


# ---------------------------------------------------------------------------
# Chart 9: temperature-sweep success rate per system, with error bars from
# the N=5 repeats -- answers "does matching temperature change the picture"
# and "is Invariants-at-temp=0 actually deterministic."
# ---------------------------------------------------------------------------

def plot_temperature_sweep(sweep_df: pd.DataFrame | None):
    if sweep_df is None or sweep_df.empty:
        print("  [values] Temperature sweep: no data found, skipping chart 09")
        return

    temps = sorted(sweep_df["Temperature"].unique())
    labels = [SYSTEM_LABELS[s] for s in SYSTEM_ORDER]
    series = {f"temp={t}": [] for t in temps}
    errs = {f"temp={t}": [] for t in temps}
    raw = {}
    for sys_ in SYSTEM_ORDER:
        raw[SYSTEM_LABELS[sys_]] = {}
        for t in temps:
            rows = sweep_df[(sweep_df["System"] == sys_) & (sweep_df["Temperature"] == t)]
            rate = 100 * rows["Semantic_Success"].mean() if len(rows) else 0
            std = 100 * rows["Semantic_Success"].std() if len(rows) > 1 else 0
            series[f"temp={t}"].append(rate)
            errs[f"temp={t}"].append(std)
            raw[SYSTEM_LABELS[sys_]][f"temp={t}"] = f"{rate:.1f}% +/- {std:.1f} (n={len(rows)})"

    print_values("Temperature sweep: success rate per system (mean +/- std across N trials)", raw)

    # Determinism check: for Invariants at temp=0.0, are all trials of a
    # given case bit-identical (same Output_Hash)?
    if "Output_Hash" in sweep_df.columns:
        det_rows = sweep_df[(sweep_df["System"] == "Invariants") & (sweep_df["Temperature"] == 0.0)]
        print("  [values] Determinism check (Invariants, temp=0.0): distinct output hashes per case")
        for case_id, grp in det_rows.groupby("Benchmark_ID"):
            distinct = grp["Output_Hash"].nunique()
            print(f"    {case_id}: {distinct} distinct hash(es) across {len(grp)} trials"
                  f"{' -- NOT bit-identical' if distinct > 1 else ' -- deterministic'}")

    n = len(temps)
    width = 0.8 / n
    x = range(len(labels))
    fig, ax = plt.subplots(figsize=(8, 5.5))
    for i, t in enumerate(temps):
        offset = (i - (n - 1) / 2) * width
        xs = [xi + offset for xi in x]
        ax.bar(xs, series[f"temp={t}"], width, yerr=errs[f"temp={t}"],
               label=f"temp={t}", capsize=3,
               color=COLOR_PLAIN if i == 0 else COLOR_BASELINE, zorder=3)
    ax.set_xticks(list(x))
    ax.set_xticklabels(labels)
    ax.set_ylabel("Cases passed (%)")
    ax.set_ylim(0, 115)
    ax.set_title(f"Temperature sweep: success rate per system\n(N={sweep_df.groupby(['System', 'Temperature']).size().max()} repeats, 6 cases, error bars = std across trials)",
                 fontsize=12, fontweight="bold")
    ax.text(0.5, -0.16,
            "Invariants' 100% here is not a general claim: none of these 6 cases happen to be a\n"
            "known dead-end case (see chart 10) -- it reflects this specific sample, not immunity.",
            transform=ax.transAxes, ha="center", va="top", fontsize=8.5,
            color=TEXT_SECONDARY, style="italic")
    ax.legend(frameon=False, loc="upper left", bbox_to_anchor=(1.01, 1.0))
    style_axes(ax)
    save(fig, "09_temperature_sweep_success_rate.png")


# ---------------------------------------------------------------------------
# Chart 10: dead-end failure vs. constraint tightness -- characterizes the
# no-backtrack failure mode across schemas_deadend_stress instead of leaving
# it as a one-off anecdote.
# ---------------------------------------------------------------------------

# window_start / upper_bound for each fixed-threshold case; the
# sibling-dependent case has no fixed ratio (the threshold depends on
# whatever the model samples for the prior field), so it's plotted apart.
DEADEND_TIGHTNESS = {
    "DED_tightness_10pct": 10 / 96,
    "DED_tightness_30pct": 30 / 96,
    "DED_tightness_50pct": 50 / 96,
    "DED_tightness_70pct": 70 / 96,
    "DED_tightness_85pct": 85 / 96,
    "DED_tightness_92pct": 92 / 96,
}

def plot_deadend_tightness(suite_json: dict):
    payload = suite_json.get("schemas_deadend_stress")
    if not payload:
        print("  [values] Dead-end stress suite: no data found, skipping chart 10")
        return

    ordered = sorted(DEADEND_TIGHTNESS.items(), key=lambda kv: kv[1])
    labels, ratios, outcomes = [], [], []
    sibling_outcome = None
    raw = {}
    for case in payload["cases"]:
        cid = case["id"]
        inv = case.get("invariants") or {}
        crashed = inv.get("raw_output") is None
        if cid in DEADEND_TIGHTNESS:
            raw[cid] = f"ratio={DEADEND_TIGHTNESS[cid]:.2f} crashed={crashed}"
        elif cid == "DED_sibling_dependent":
            sibling_outcome = crashed
            raw[cid] = f"ratio=N/A (sibling-dependent) crashed={crashed}"

    for cid, ratio in ordered:
        case = next((c for c in payload["cases"] if c["id"] == cid), None)
        if case is None:
            continue
        inv = case.get("invariants") or {}
        crashed = inv.get("raw_output") is None
        labels.append(f"{ratio:.2f}")
        ratios.append(ratio)
        outcomes.append(0 if crashed else 1)

    print_values("Dead-end stress: outcome per case (1 = Invariants completed, 0 = crashed / mask dead-end)", raw)

    # This is a categorical (crashed / completed) outcome, not a magnitude --
    # encoding it as bar HEIGHT (0 vs 1) makes the crashed cases invisible
    # (a zero-height bar reads as "no data"). Every bar is drawn at the same
    # height instead, with color + an explicit text label carrying the
    # outcome, so a crash is exactly as visible as a pass.
    if sibling_outcome is not None:
        labels = [*labels, "sibling-\ndependent"]
        outcomes = [*outcomes, 0 if sibling_outcome else 1]

    fig, ax = plt.subplots(figsize=(9, 5))
    colors = [COLOR_INVARIANTS if o == 1 else COLOR_DIV_SLOWER for o in outcomes]
    ax.bar(labels, [1] * len(labels), width=0.5, color=colors, zorder=3)
    for i, o in enumerate(outcomes):
        ax.text(i, 0.5, "Completed" if o == 1 else "Crashed", ha="center", va="center",
                fontsize=10, fontweight="bold", color="white", rotation=90)
    ax.set_xlabel("Window-start / upper-bound ratio (higher = tighter, single-run each)")
    ax.set_yticks([])
    ax.set_title("No-backtrack dead-end vs. constraint tightness\n(N=1 per case -- a fixed threshold only fails once it excludes the\nmodel's own greedy default; sibling-dependent fails regardless of ratio)",
                 fontsize=12, fontweight="bold")
    style_axes(ax, y_grid=False)
    save(fig, "10_deadend_tightness.png")


# ---------------------------------------------------------------------------
# Chart 11: throughput on the long-free-text suite specifically -- the
# deliberate worst case for this architecture (mask overhead paid on every
# token with no bypass or rejection to compensate). Shown honestly even if
# baseline wins here.
# ---------------------------------------------------------------------------

def plot_long_freetext_throughput(df: pd.DataFrame):
    sub = df[df["Suite"] == "schemas_long_freetext"]
    if sub.empty:
        print("  [values] Long free-text suite: no data found, skipping chart 11")
        return

    case_ids = sorted(sub["Benchmark_ID"].unique())
    series = {sys_: [] for sys_ in SYSTEM_ORDER}
    raw = {}
    for cid in case_ids:
        raw[cid] = {}
        for sys_ in SYSTEM_ORDER:
            rows = sub[(sub["Benchmark_ID"] == cid) & (sub["System"] == sys_)]
            val = rows["Tokens_Per_Sec"].mean() if len(rows) else 0
            series[sys_].append(val)
            raw[cid][sys_] = round(val, 2)

    print_values("Long free-text suite: mean tokens/sec by case (worst case for mask overhead)", raw)

    fig, ax = plt.subplots(figsize=(9, 5))
    grouped_bars(ax, case_ids, series, SYSTEM_COLORS, SYSTEM_LABELS,
                 value_fmt=lambda v: f"{v:.1f}")
    ax.set_ylabel("Tokens / sec")
    ax.set_title("Raw throughput on long, unconstrained free-text fields\n(no bypass possible here -- mask overhead has nothing to compensate it)",
                 fontsize=12, fontweight="bold")
    ax.legend(frameon=False, loc="upper left", bbox_to_anchor=(1.01, 1.0))
    style_axes(ax)
    save(fig, "11_long_freetext_throughput.png")


def main():
    print("Loading data...")
    df = load_all_results()
    suite_json = load_suite_json()
    mask_json = load_mask_timing_json()
    sweep_df = load_temperature_sweep()
    print(f"  all_results.csv: {len(df)} rows across {df['Suite'].nunique()} suites")
    print(f"  detailed suite JSON: {len(suite_json)} suites")
    print(f"  mask-timing JSON: {len(mask_json)} suites")
    print(f"  temperature sweep: {'none found' if sweep_df is None else f'{len(sweep_df)} rows'}")

    print("\nGenerating charts...")
    plot_field_level_pass_rate_by_suite(suite_json)
    plot_success_rate_by_suite(df)
    plot_assertion_pass_rate_overall(suite_json)
    plot_assertion_pass_rate_by_type(suite_json)
    plot_bypass_rate_by_suite(mask_json)
    plot_mean_metric_by_suite(df, "Tokens_Generated", "Mean tokens generated per case",
                              "Tokens generated per case, by suite", "05_tokens_generated.png")
    plot_mean_metric_by_suite(df, "Wall_Time_s", "Mean wall time per case (s)",
                              "Wall-clock time per case, by suite", "06_wall_time_comparison.png")
    plot_wall_time_ratio(df)
    plot_mask_overhead_share(mask_json)
    plot_temperature_sweep(sweep_df)
    plot_deadend_tightness(suite_json)
    plot_long_freetext_throughput(df)

    print(f"\nDone. Charts written to {PLOTS_DIR}/")


if __name__ == "__main__":
    main()
