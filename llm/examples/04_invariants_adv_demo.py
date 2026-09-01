from invariants.Engine import Engine
from invariants.Processor import ConstrainedGenerator


def main():
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

    print("Initializing LLM Engine...")
    engine = Engine()
    generator = ConstrainedGenerator(engine)

    system_prompt = (
        "You are an automated provisioning orchestrator configuring an enterprise Kubernetes cluster quote. "
        "Output ONLY valid JSON."
    )

    print("\nRunning Generator...")
    result = generator.generate(
        source, "CloudClusterQuote", system_prompt, verbose=True
    )

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
