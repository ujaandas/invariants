import pytest
from invariants.Engine import Engine
from invariants.Buffer import FieldBuffer


@pytest.fixture(scope="module")
def engine():
    print("Loading model...")
    return Engine()


def test_buffer_initialization(engine):
    buffer = FieldBuffer(engine)
    assert len(buffer) == 0
    assert buffer.current_text() == ""


def test_speculative_decoding(engine):
    buffer = FieldBuffer(engine)

    token_1 = engine.tokenize("1")[0]
    token_5 = engine.tokenize("5")[0]

    speculated_text = buffer.speculative_decode(token_1)
    assert speculated_text == "1"
    assert len(buffer) == 0

    buffer.commit_token(token_1)
    assert len(buffer) == 1
    assert buffer.current_text() == "1"

    speculated_text_2 = buffer.speculative_decode(token_5)
    assert speculated_text_2 == "15"
    assert len(buffer) == 1

    buffer.commit_token(token_5)
    assert len(buffer) == 2
    assert buffer.current_text() == "15"


def test_buffer_clearing(engine):
    buffer = FieldBuffer(engine)

    token_1 = engine.tokenize("1")[0]
    buffer.commit_token(token_1)
    assert len(buffer) == 1

    buffer.clear()
    assert len(buffer) == 0
    assert buffer.current_text() == ""
