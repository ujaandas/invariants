import invariants_cpp
from invariants.Engine import Engine
from invariants.Buffer import FieldBuffer
from invariants.Processor import ConstraintProcessor


def main():
    # Load the LLM and the C++ state mach
    engine = Engine()
    rt = invariants_cpp.Runtime()
    rt.reset()

    # Build JSON string sequentially
    final_json = "{\n"
    print("Starting generation \n")
    print("{")

    # Orchestrator loop
    while rt.has_more_fields():
        field_name = rt.get_active_field_name()

        # Structurally write JSON key
        final_json += f'  "{field_name}": '
        print(f'  "{field_name}": ', end="", flush=True)

        # Check for deterministic bypass
        if rt.is_active_field_deterministic():
            # Calculate the answer instantly
            val_str = rt.solve_deterministic()
            final_json += val_str
            print(f"{val_str} (Bypassed)", flush=True)

        else:
            # Delegate to LLM
            buffer = FieldBuffer(engine)
            processor = ConstraintProcessor(rt, buffer)

            # Feed the LLM the exact JSON built so far so it knows the context
            state = engine.prefill(final_json, logits_processor=processor)

            generated_val = ""
            while True:
                token = engine.step(state)
                if token is None:
                    break

                char_chunk = engine.decode([token])

                exit_chars = [",", "\n", "}"]
                if any(c in char_chunk for c in exit_chars):
                    # Salvage text before the exit character
                    for c in exit_chars:
                        if c in char_chunk:
                            char_chunk = char_chunk.split(c)[0]
                            break

                    generated_val += char_chunk
                    print(char_chunk, end="", flush=True)
                    break  # Now we break safely

                buffer.commit_token(token)
                generated_val += char_chunk
                print(char_chunk, end="", flush=True)

            final_json += generated_val
            # Submit value to C++ to update global state
            clean_val = generated_val.strip()

            # If string field, remove JSON quotes before submitting to C++
            if rt.get_active_field_type() == invariants_cpp.FieldType.String:
                if clean_val.startswith('"') or clean_val.startswith("'"):
                    clean_val = clean_val[1:]
                if clean_val.endswith('"') or clean_val.endswith("'"):
                    clean_val = clean_val[:-1]

            # Submit raw value back to C++ to update global state
            rt.submit_val_str(field_name, clean_val)

            print(" (LLM generated)", flush=True)

        # Add structural commas and newlines
        if rt.has_more_fields():
            final_json += ",\n"
            print(",")
        else:
            final_json += "\n"
            print()

    final_json += "}"
    print("}")

    print("Generation complete!")
    print(final_json)


if __name__ == "__main__":
    main()
