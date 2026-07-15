import invariants

# uv run --with pytest pytest lang/tests/


def test_enums():
    assert invariants.ValidationStatus.Valid.name == "Valid"
    assert invariants.FieldType.Integer.name == "Integer"


def test_speculative_character_generation_simulation():
    rt = invariants.Runtime()

    assert rt.validate_active_field_partial("1") == invariants.ValidationStatus.Valid
    assert rt.validate_active_field_partial("15") == invariants.ValidationStatus.Valid
    assert (
        rt.validate_active_field_partial("15.")
        == invariants.ValidationStatus.PartialValid
    )
    assert rt.validate_active_field_partial("15.5") == invariants.ValidationStatus.Valid
    assert (
        rt.validate_active_field_partial("15.50") == invariants.ValidationStatus.Valid
    )

    rt.submit_val_str("unit_price", "15.50")
    assert rt.get_active_field_name() == "quantity"

    assert rt.validate_active_field_partial("2") == invariants.ValidationStatus.Valid
    assert rt.validate_active_field_partial("25") == invariants.ValidationStatus.Valid
    assert rt.validate_active_field_partial("250") == invariants.ValidationStatus.Valid

    rt.submit_val_str("quantity", "250")
    assert rt.get_active_field_name() == "currency"

    assert (
        rt.validate_active_field_partial("U")
        == invariants.ValidationStatus.PartialValid
    )
    assert (
        rt.validate_active_field_partial("US")
        == invariants.ValidationStatus.PartialValid
    )
    assert rt.validate_active_field_partial("USD") == invariants.ValidationStatus.Valid

    rt.submit_val_str("currency", "USD")
    assert rt.get_active_field_name() == "total_price"

    assert rt.is_active_field_deterministic() is True
    calculated_total = rt.solve_deterministic()

    assert float(calculated_total) == 3875.0
    assert rt.has_more_fields() is False
