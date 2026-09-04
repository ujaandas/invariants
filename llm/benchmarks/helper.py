import json
from dataclasses import dataclass
from typing import Any


@dataclass
class Prompts:
    system: str
    user: str


@dataclass
class EvalAssertion:
    type: str
    field: str
    # Range specific
    min: float | None = None
    max: float | None = None
    # Membership specific
    choices: list[Any] | None = None
    # Exact value specific
    expected: Any | None = None

    @classmethod
    def from_dict(cls, data: dict) -> "EvalAssertion":
        return cls(**data)

    def evaluate(self, generated_json: dict) -> bool:
        return self.evaluate_detailed(generated_json)[0]

    def evaluate_detailed(self, generated_json: dict) -> tuple[bool, str]:
        """Same check as evaluate(), but also returns a human-readable
        explanation of what was compared -- this is what gets persisted to
        the benchmark results log/JSON so a failure is legible without
        re-running the case."""
        if self.field not in generated_json:
            return False, f"field '{self.field}' missing from output"

        val = generated_json[self.field]

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
                # e.g. "math" assertions (used from L2 onward) aren't
                # implemented yet -- surface that plainly instead of a
                # silent/misleading "Failed", since a wave of these could
                # otherwise look like real generation failures in the data.
                return False, f"assertion type '{self.type}' not yet implemented"
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
