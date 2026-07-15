import pytest
import numpy as np

import invariants_cpp
from invariants.Engine import Engine
from invariants.Buffer import FieldBuffer
from invariants.Processor import ConstraintProcessor


@pytest.fixture(scope="module")
def engine():
    print("Loading model...")
    return Engine()


def test_processor_masks_invalid_types(engine):
    rt = invariants_cpp.Runtime()
    rt.reset()

    rt.submit_val_str("unit_price", "10.0")
    assert rt.get_active_field_name() == "quantity"
    assert rt.get_active_field_type() == invariants_cpp.FieldType.Integer

    buffer = FieldBuffer(engine)
    processor = ConstraintProcessor(rt, buffer)

    vocab_size = engine.llm.n_vocab()
    mock_scores = np.full(vocab_size, -10.0, dtype=np.single)

    token_A = engine.tokenize("A")[0]
    token_5 = engine.tokenize("5")[0]

    mock_scores[token_A] = 100.0
    mock_scores[token_5] = 10.0

    masked_scores = processor(input_ids=[], scores=mock_scores)

    assert masked_scores[token_A] == -np.inf
    assert masked_scores[token_5] == 10.0

    chosen_token = int(np.argmax(masked_scores))
    assert chosen_token == token_5
