import json
from dataclasses import dataclass
from typing import Any


def eval_math_expr(expr: str, scope: dict) -> bool:
    # expr strings come from schema files we author ourselves, so a plain
    # eval() is fine -- builtins stripped, only `abs` exposed.
    return bool(eval(expr, {"__builtins__": {}, "abs": abs}, scope))  # noqa: S307


@dataclass
class Prompts:
    system: str
    user: str


@dataclass
class EvalAssertion:
    type: str
    # None for "math" assertions, which reference fields via `expr` instead
    field: str | None = None
    # Range specific
    min: float | None = None
    max: float | None = None
    # Membership specific
    choices: list[Any] | None = None
    # Exact value specific
    expected: Any | None = None
    # Math specific: a Python expression, e.g. "abs(a - b) < 0.01"
    expr: str | None = None

    @classmethod
    def from_dict(cls, data: dict) -> "EvalAssertion":
        return cls(**data)

    def evaluate(self, generated_json: dict) -> bool:
        return self.evaluate_detailed(generated_json)[0]

    def _resolve_field(self, generated_json: dict) -> tuple[bool, Any]:
        # Walks self.field (e.g. "profile.vcpu_cores") through nested dicts.
        current: Any = generated_json
        for part in (self.field or "").split("."):
            if not isinstance(current, dict) or part not in current:
                return False, None
            current = current[part]
        return True, current

    def evaluate_detailed(self, generated_json: dict) -> tuple[bool, str]:
        # Same as evaluate(), plus a human-readable explanation for logs.
        if self.type == "math":
            if not self.expr:
                return False, "math assertion missing 'expr'"
            try:
                ok = eval_math_expr(self.expr, generated_json)
            except Exception as e:
                return False, f"error evaluating '{self.expr}': {e}"
            return ok, f"{self.expr} -> {ok}"

        found, val = self._resolve_field(generated_json)
        if not found:
            return False, f"field '{self.field}' missing from output"

        try:
            if self.type == "range":
                ok = self.min <= float(val) <= self.max
                return ok, f"{val!r} in [{self.min}, {self.max}]"
            elif self.type == "membership":
                ok = val in self.choices
                return ok, f"{val!r} in {self.choices}"
            elif self.type == "exact_value":
                ok = val == self.expected
                return ok, f"{val!r} == {self.expected!r}"
            else:
                return False, f"unknown assertion type '{self.type}'"
        except (ValueError, TypeError) as e:
            # E.g., trying to float() a string that the LLM hallucinated
            return False, f"error evaluating against {val!r}: {e}"


@dataclass
class BenchmarkCase:
    id: str
    name: str
    domain: str
    root_spec: str
    description: str
    prompts: Prompts
    invariants_dsl: str
    json_schema: dict[str, Any]
    eval_assertions: list[EvalAssertion]

    @classmethod
    def from_dict(cls, data: dict) -> "BenchmarkCase":
        return cls(
            id=data["id"],
            name=data["name"],
            domain=data["domain"],
            root_spec=data["root_spec"],
            description=data["description"],
            prompts=Prompts(**data["prompts"]),
            invariants_dsl=data["invariants_dsl"],
            json_schema=data["json_schema"],
            eval_assertions=[
                EvalAssertion.from_dict(a) for a in data["eval_assertions"]
            ],
        )


@dataclass
class BenchmarkSuite:
    version: str
    level_description: str
    benchmarks: dict[str, BenchmarkCase]

    @classmethod
    def from_dict(cls, data: dict) -> "BenchmarkSuite":
        benchmarks = {
            k: BenchmarkCase.from_dict(v) for k, v in data["benchmarks"].items()
        }
        return cls(
            version=data.get("version", "1.0"),
            level_description=data.get("level_description", ""),
            benchmarks=benchmarks,
        )

    @classmethod
    def load_from_file(cls, filepath: str) -> "BenchmarkSuite":
        with open(filepath, "r", encoding="utf-8") as f:
            return cls.from_dict(json.load(f))
