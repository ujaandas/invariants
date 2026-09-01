import glob
import json
import os
import sys

try:
    import invariants_cpp
except ImportError:
    print("Could not import invariants_cpp.")
    sys.exit(1)


def main():
    # Find all schema files matching the pattern
    schema_files = glob.glob("benchmarks/schemas_l*.json")

    if not schema_files:
        print("No schema_l*.json files found in the current directory.")
        sys.exit(1)

    all_passed = True
    total_checked = 0
    total_failed = 0

    print(f"{len(schema_files)} schema file(s). Starting C++ AST compilation check..")

    for file_path in sorted(schema_files):
        print(f"--- Verifying {os.path.basename(file_path)} ---")

        with open(file_path, "r", encoding="utf-8") as f:
            try:
                data = json.load(f)
            except json.JSONDecodeError as e:
                print(f"  JSON syntax error in {file_path}: {e}")
                all_passed = False
                continue

        benchmarks = data.get("benchmarks", {})

        for bench_id, benchmark in benchmarks.items():
            total_checked += 1
            dsl_source = benchmark.get("invariants_dsl", "")
            root_spec = benchmark.get("root_spec", "")

            try:
                _ = invariants_cpp.EngineSession(dsl_source, root_spec)
                print(f"  {bench_id} (Root: {root_spec})")
            except Exception as e:  # noqa: BLE001
                total_failed += 1
                all_passed = False
                print(f"  {bench_id} (Root: {root_spec})")
                print(f"    Compiler Error: {e}")
                print(
                    f"         --- Source Snippet ---\n{dsl_source}\n         ----------------------"
                )

    print("--- Summary ---")
    if all_passed:
        print(
            f"All {total_checked} schemas successfully compiled through the C++ AST and Topological Sorter!"
        )
    else:
        print(
            f" {total_failed}/{total_checked} schemas failed C++ compilation. See errors above"
        )
        sys.exit(1)


if __name__ == "__main__":
    main()
