import invariants_cpp

# uv run --with pytest pytest lang/tests/

def test_enums():
    assert invariants_cpp.ValidationStatus.Valid.name == "Valid"

def test_speculative_character_generation_simulation():
    # Define the spec
    source = """
    spec Order {
        field unit_price: Number {}
        field quantity: Integer {}
        field currency: String {
            value == "USD";
        }
        field total_price: Number {
            value == this.unit_price * this.quantity;
        }
    }
    """

    # Compile the spec and extract the runtime
    session = invariants_cpp.EngineSession(source, "Order")
    rt = session.runtime

    session = invariants_cpp.EngineSession(source, "Order")
    rt = session.runtime

    assert rt.get_active_field_name() == "unit_price"
    assert rt.validate_partial("1") == invariants_cpp.ValidationStatus.Valid
    assert rt.validate_partial("15") == invariants_cpp.ValidationStatus.Valid
    
    # C++ std::stod parses "15." successfully as 15.0, so it is Valid
    assert rt.validate_partial("15.") == invariants_cpp.ValidationStatus.Valid
    assert rt.validate_partial("15.5") == invariants_cpp.ValidationStatus.Valid
    
    # A lone "-" cannot be parsed by stod, throwing an exception -> PartialValid
    assert rt.validate_partial("-") == invariants_cpp.ValidationStatus.PartialValid

    rt.submit_val_str("unit_price", "15.50")
    
    assert rt.get_active_field_name() == "quantity"
    assert rt.validate_partial("2") == invariants_cpp.ValidationStatus.Valid
    assert rt.validate_partial("25") == invariants_cpp.ValidationStatus.Valid
    assert rt.validate_partial("250") == invariants_cpp.ValidationStatus.Valid

    rt.submit_val_str("quantity", "250")
    
    assert rt.get_active_field_name() == "currency"
    
    # Currency is an exact assignment (value == "USD"), so the engine solves it instantly
    assert rt.is_active_field_deterministic() is True
    assert rt.solve_deterministic() == "USD"
    
    assert rt.get_active_field_name() == "total_price"
    
    # Total price is also solved instantly
    assert rt.is_active_field_deterministic() is True
    calculated_total = rt.solve_deterministic()
    
    assert float(calculated_total) == 3875.0
    assert rt.has_more_fields() is False