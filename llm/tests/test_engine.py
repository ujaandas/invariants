import pytest
from invariants.Engine import Engine


@pytest.fixture(scope="module")
def engine():
    print("Loading model...")
    return Engine()


def test_engine_initialization(engine):
    assert engine.llm is not None
    assert hasattr(engine, "prefill")
    assert hasattr(engine, "step")


def test_tokenization_and_decoding(engine):
    text = "Hello, world!"
    tokens = engine.tokenize(text)

    assert len(tokens) > 0
    assert isinstance(tokens[0], int)

    decoded = engine.decode(tokens)
    assert decoded.strip() == text


def test_stateful_generation_loop(engine):
    prompt = "<|im_start|>user\nWho are you?<|im_end|>\n<|im_start|>assistant\n"
    state = engine.prefill(prompt)

    assert state is not None
    assert state.generated_tokens == []

    step_count = 10
    for _ in range(step_count):
        token = engine.step(state)
        if token is None:
            break

    assert len(state.generated_tokens) > 0
    assert len(state.generated_tokens) <= step_count

    decoded_output = engine.decode(state.generated_tokens)
    assert isinstance(decoded_output, str)
    assert len(decoded_output) > 0

    print(f"\n[Test Output] LLM generated: '{decoded_output}'")
