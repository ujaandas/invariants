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

}  // namespace invariants::runtime