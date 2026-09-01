from invariants.Engine import Engine
from invariants.Processor import ConstrainedGenerator


def main():
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

    print("Initializing LLM Engine...")
    engine = Engine()
    generator = ConstrainedGenerator(engine)

    system_prompt = (
        "You are an automated logistics system generating a JSON invoice for a shipment of Industrial Titanium. "
        "Output ONLY valid JSON."
    )

    print("\nRunning Generator...")
    result = generator.generate(source, "LogisticsInvoice", system_prompt, verbose=True)

    print("\033[1m--- Generation Benchmark Metrics ---\033[0m")
    print(f"Total Fields Resolved:       {result.total_fields}")
    print(f"Fields Bypassed (Zero GPU):  {result.fields_bypassed}")
    print(f"LLM Tokens Sampled:          {result.tokens_sampled}")
    print(f"Total Generation Wall Time:  {result.wall_time_seconds:.3f}s")
    if result.wall_time_seconds > 0:
        print(
            f"Constrained Decode Speed:    {result.tokens_sampled / result.wall_time_seconds:.2f} tok/s"
        )


if __name__ == "__main__":
    main()
