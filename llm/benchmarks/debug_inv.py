import json

from invariants.Engine import Engine
from invariants.Processor import ConstrainedGenerator


def main():
    source = """
    spec UserOnboarding {
        field username: String {}
        field age: Integer {
             value >= 18;
             value <= 120;
         }
        field country_code: String {
            value IN ["US", "GB", "DE", "FR", "JP"];
        }
        field is_verified: Boolean {
            value == true;
        }
        field terms_accepted: Boolean {
            value == true;
        }
    }
    """

    print("Initializing LLM Engine...")
    engine = Engine()
    generator = ConstrainedGenerator(engine)

    system_prompt = (
        "You are a backend identity verification service configuring user account profiles. "
        "Output ONLY raw, valid JSON."
    )
    user_prompt = "Generate a compliant user onboarding record for an adult customer residing in a supported jurisdiction."

    full_prompt = f"{system_prompt}\n\n{user_prompt}"

    print("Running Generator (Verbose Mode ON)...")

    # verbose=True will print the step-by-step LLM generations and C++ validations
    result = generator.generate(source, "UserOnboarding", full_prompt, verbose=True)

    print("--- Raw Output ---")
    print(result.json_output)

    print("--- Parsed Output Types ---")
    try:
        data = json.loads(result.json_output)
        print(json.dumps(data, indent=2))
        print(f"\nType of country_code: {type(data.get('country_code')).__name__}")
        print(f"Value of country_code: {data.get('country_code')!r}")
    except json.JSONDecodeError:
        print("Failed to parse JSON.")


if __name__ == "__main__":
    main()
