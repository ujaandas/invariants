from invariants.Engine import Engine
import pytest
import torch

MODEL = "Qwen/Qwen2.5-0.5B-Instruct"


@pytest.fixture(scope="module")
def engine():
    return Engine(
        model_name=MODEL,
        device="cuda" if torch.cuda.is_available() else "cpu",
        quantize=False,  # For stability
    )


def test_prefill_runs(engine: Engine):
    tokenizer = engine.tokenizer

    input_ids = tokenizer(
        "Hello world",
        return_tensors="pt",
    )["input_ids"]

    state = engine.prefill(input_ids)

    assert state is not None
    assert state.past_kv is not None
    assert state.logits is not None
