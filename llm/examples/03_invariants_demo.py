import invariants_cpp
from invariants.Buffer import FieldBuffer
from invariants.Engine import Engine
from invariants.Processor import ConstraintProcessor


def main():
    # Complex schema to test topological reordering and deterministic cascades
    source = """
    spec LogisticsInvoice {
        field cargo_name: String {}
        field weight_kg: Number {}
        field price_per_kg: Number {}
        
        field base_cost: Number {
            value == this.weight_kg * this.price_per_kg;
        }
        field import_tariff: Number {
            value == this.base_cost * 0.15;
        }
        field total_cost: Number {
            value == this.base_cost + this.import_tariff;
        }

        invariant valid_weight {
            this.weight_kg > 0.0;
            this.weight_kg <= 5000.0;
        }
    }
    """

    print("Loading LLM and compiling C++ AST...")
    engine = Engine()

    # Initialize the new session wrapper
    session = invariants_cpp.EngineSession(source, "LogisticsInvoice")
    rt = session.runtime

    # Prompt engineering to steer the LLM
    system_prompt = (
        "You are an automated logistics system generating a JSON invoice for a shipment of Industrial Titanium. "
        "Output ONLY valid JSON."
    )

    final_json = "{\n"
    print("\n--- Starting Constrained Generation ---\n{")

    while rt.has_more_fields():
        field_name = rt.get_active_field_name()

        final_json += f'  "{field_name}": '
        print(f'  "{field_name}": ', end="", flush=True)

        if rt.is_active_field_deterministic():
            val_str = rt.solve_deterministic()

            # Formatting calculated strings for JSON
            if (
                val_str == "true"
                or val_str == "false"
                or val_str.replace(".", "", 1).isdigit()
            ):
                json_val = val_str
            else:
                json_val = f'"{val_str}"'

            final_json += json_val
            print(f"{json_val}  \033[92m(C++ Bypassed)\033[0m", flush=True)

        else:
            buffer = FieldBuffer(engine)
            processor = ConstraintProcessor(rt, buffer, engine)

            # Pass the current JSON state as context
            state = engine.prefill(
                f"{system_prompt}\n{final_json}", logits_processor=processor
            )

            generated_val = ""
            while True:
                token = engine.step(state)
                if token is None:
                    break

                char_chunk = engine.decode([token])

                # Structural exit detection
                exit_chars = [",", "\n", "}"]
                if any(c in char_chunk for c in exit_chars):
                    for c in exit_chars:
                        if c in char_chunk:
                            char_chunk = char_chunk.split(c)[0]
                            break

                    generated_val += char_chunk
                    print(char_chunk, end="", flush=True)
                    break

                buffer.commit_token(token)
                generated_val += char_chunk
                print(char_chunk, end="", flush=True)

            final_json += generated_val

            # C++ parseLLMString automatically handles quote stripping for Strings
            clean_val = generated_val.strip().rstrip(",\n} ")
            rt.submit_val_str(field_name, clean_val)
            print("  \033[94m(LLM Generated)\033[0m", flush=True)

        if rt.has_more_fields():
            final_json += ",\n"
            print(",")
        else:
            final_json += "\n"
            print()

    final_json += "}"
    print("}\n\nGeneration complete!")


if __name__ == "__main__":
    main()
