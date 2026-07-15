#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <variant>
namespace invariants::runtime {

// Keep these simple for now
enum class FieldType : std::uint8_t { Integer, Number, String };

enum class ValidationStatus : std::uint8_t {
  Valid,         // Fully satisfies all constraints
  PartialValid,  // Incomplete, but not yet valid (e.g. mid-generation)
  Invalid        // Actively violates a constraint
};

// Raw parsed value
using Value = std::variant<int, double, std::string>;

using Environment = std::unordered_map<std::string, Value>;

// Compilation target for FieldSymbol
struct FieldNode {
  std::string name;
  FieldType type;
  bool is_deterministic = false;  // True if WHOLLY dependent on other fields
  // (i.e. do we need to generate this at all?)

  // Validates a partially generated string against this field's rules
  std::function<ValidationStatus(std::string_view, const Environment&)>
      validate;

  // If deterministic, compute value using environment
  std::function<Value(const Environment&)> compute_value;
};

class Runtime {
 private:
  std::unordered_map<std::string, FieldNode> fields;
  std::vector<std::string> genOrder;  // Hand-rolled topological sort order

  // Active State
  Environment environment;
  size_t currStepIdx = 0;

  // Setup helper to define our "mocked" BulkOrder schema
  void initSchema();

 public:
  Runtime();

  // DAG nav n flow control
  bool hasMoreFields() const;
  std::string getActiveFieldName() const;
  FieldType getActiveFieldType() const;
  std::vector<std::string> getGenOrder() const;

  // Call when finish generating field to mutate state
  void submitVal(std::string_view, const Value&);
  void submitVal(std::string_view, std::string_view);

  // Solvers
  bool isAciveFieldDeterministic() const;
  std::string
  solveDeterministic();  // Computes, saves, and returns string representation

  // Checks if appending 'proposedChars' keeps active field in safety
  // constraints
  ValidationStatus validate_active_field_partial(std::string_view) const;

  // Inspect
  const Environment& get_environment() const;
  void reset();
};

}  // namespace invariants::runtime