import time

import invariants_cpp
from invariants.Buffer import FieldBuffer
from invariants.Engine import Engine
from invariants.Processor import ConstraintProcessor


def main():
    # 1. Multi-spec schema designed to rigorously test all compiler phases
    source = """
    spec NodeProfile {
        field vcpu_cores: Number {}
        field ram_gb: Number {}
        
        invariant node_hardware_balance {
            this.vcpu_cores >= 2.0;
            this.vcpu_cores <= 64.0;
            this.ram_gb >= this.vcpu_cores * 2.0;
            this.ram_gb <= 256.0;
        }
    }

    spec StorageProfile {
        field storage_tb: Number {}
        field redundancy: String {
            value == "Geo-Redundant";
        }
        
        invariant capacity_limits {
            this.storage_tb >= 1.0;
            this.storage_tb <= 100.0;
        }
    }

    spec CloudClusterQuote {
        // Declared first to mandate Kahn's topological reordering
        field total_monthly_bill: Number {
            value == this.compute_cost + this.storage_cost;
        }
        
        field compute_cost: Number {
            value == (this.profile.vcpu_cores * 15.0 + this.profile.ram_gb * 4.0) * this.node_count;
        }
        
        field storage_cost: Number {
            value == this.storage.storage_tb * 65.0;
        }

        field node_count: Number {}
        field profile: NodeProfile {}
        field storage: StorageProfile {}

        invariant cluster_scale {
            this.node_count >= 2.0;
            this.node_count <= 16.0;
        }
    }
    """

    print("\033[1m[1/3] Initializing LLM Engine & Pre-Caching Vocabulary...\033[0m")
    start_init = time.perf_counter()
    engine = Engine()
    init_time = time.perf_counter() - start_init
    print(f"Engine ready in {init_time:.2f}s")

    print(
        "\n\033[1m[2/3] Compiling Invariants AST & Building Topological Schedule...\033[0m"
    )
    session = invariants_cpp.EngineSession(source, "CloudClusterQuote")
    rt = session.runtime

    system_prompt = (
        "You are an automated provisioning orchestrator configuring an enterprise Kubernetes cluster quote. "
        "Output ONLY valid JSON."
    )

    final_json = "{\n"
    print("\n\033[1m[3/3] Starting Constrained Generation Execution Graph\033[0m\n{")

    llm_tokens_generated = 0
    bypassed_fields = 0
    llm_generation_time = 0.0

    while rt.has_more_fields():
        field_name = rt.get_active_field_name()

        final_json += f'  "{field_name}": '
        print(f'  "{field_name}": ', end="", flush=True)

        if rt.is_active_field_deterministic():
            bypassed_fields += 1
            val_str = rt.solve_deterministic()

            # Format outputs for valid JSON output
            if val_str in ("true", "false") or val_str.replace(".", "", 1).isdigit():
                json_val = val_str
            else:
                json_val = f'"{val_str}"'

            final_json += json_val
            print(f"{json_val}  \033[92m[C++ Bypassed & Calculated]\033[0m", flush=True)

        else:
            buffer = FieldBuffer(engine)
            processor = ConstraintProcessor(rt, buffer, engine)

            t0 = time.perf_counter()
            state = engine.prefill(
                f"{system_prompt}\n{final_json}", logits_processor=processor
            )

            generated_val = ""
            while True:
                token = engine.step(state)
                if token is None or token < 0 or token >= engine.llm.n_vocab():
                    break

                llm_tokens_generated += 1
                char_chunk = engine.decode([token])

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

            llm_generation_time += time.perf_counter() - t0
            final_json += generated_val

            clean_val = generated_val.strip().rstrip(",\n} ")
            rt.submit_val_str(field_name, clean_val)
            print("  \033[94m[LLM Sampled (Invariants Masked)]\033[0m", flush=True)

        if rt.has_more_fields():
            final_json += ",\n"
            print(",")
        else:
            final_json += "\n"
            print()

    final_json += "}"
    print("}\n")

    # Evaluation telemetry
    print("\033[1m--- Generation Benchmark Metrics ---\033[0m")
    print(
        f"Total Fields Resolved:       {bypassed_fields + (len(final_json.splitlines()) - bypassed_fields - 2)}"
    )
    print(f"Fields Bypassed (Zero GPU):  {bypassed_fields}")
    print(f"LLM Tokens Sampled:          {llm_tokens_generated}")
    print(f"Total Generation Wall Time:  {llm_generation_time:.3f}s")
    if llm_generation_time > 0:
        print(
            f"Constrained Decode Speed:    {llm_tokens_generated / llm_generation_time:.2f} tok/s"
        )


if __name__ == "__main__":
    main()
