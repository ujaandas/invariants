#include "runtime.hpp"

#include <algorithm>
#include <cmath>

using namespace invariants::runtime;

namespace {

bool isDigitsOnly(std::string_view s) {
  if (s.empty()) return false;
  return std::all_of(s.begin(), s.end(), ::isdigit);
}

// Parses double, returning true if it's a completely valid float/double
bool tryParseDouble(std::string_view s, double& out) {
  if (s.empty()) return false;
  try {
    std::string temp(s);  // Requires a null-terminated string
    size_t processed_chars = 0;
    out = std::stod(temp, &processed_chars);
    return processed_chars == s.size();
  } catch (const std::invalid_argument&) {
    return false;
  } catch (const std::out_of_range&) {
    return false;
  }
}

// Parses int, returning true if completely valid
bool tryParseInt(std::string_view s, int& out) {
  if (s.empty()) return false;
  try {
    std::string temp(s);
    size_t processed_chars = 0;
    out = std::stoi(temp, &processed_chars);
    return processed_chars == s.size();
  } catch (const std::invalid_argument&) {
    return false;
  } catch (const std::out_of_range&) {
    return false;
  }
}

// Checks if a string could eventually become a valid double (for prefix checks)
bool isValidDoublePrefix(std::string_view s) {
  if (s.empty()) return true;
  if (s == "+" || s == "-") return true;

  // Check if it contains at most one dot and only digits/signs
  bool has_dot = false;
  size_t start = (s[0] == '-' || s[0] == '+') ? 1 : 0;
  if (start == s.size()) return false;  // just a sign is fine, handled above

  for (size_t i = start; i < s.size(); ++i) {
    if (s[i] == '.') {
      if (has_dot) return false;  // multiple dots
      has_dot = true;
    } else if (!::isdigit(s[i])) {
      return false;
    }
  }
  return true;
}

}  // namespace

void Runtime::initSchema() {
  // Hard-coded - only total_price depends on unit_price and quantity
  genOrder = {"unit_price", "quantity", "currency", "total_price"};

  /*
    field unit_price: Number {
        value > 0.0;
    }
  */
  fields["unit_price"] =
      FieldNode{.name = "unit_price",
                .type = FieldType::Number,
                .is_deterministic = false,
                .validate =
                    [](std::string_view text, const Environment&) {
                      if (text.empty()) return ValidationStatus::PartialValid;
                      if (!isValidDoublePrefix(text))
                        return ValidationStatus::Invalid;

                      double val;
                      if (tryParseDouble(text, val)) {
                        // Constraint: value > 0.0
                        return (val > 0.0) ? ValidationStatus::Valid
                                           : ValidationStatus::Invalid;
                      }
                      return ValidationStatus::PartialValid;
                    },
                .compute_value = nullptr};

  /*
    field quantity: Integer {
        value >= 1;
        value <= 1000;
    }
  */
  fields["quantity"] =
      FieldNode{.name = "quantity",
                .type = FieldType::Integer,
                .is_deterministic = false,
                .validate =
                    [](std::string_view text, const Environment&) {
                      if (text.empty()) return ValidationStatus::PartialValid;
                      if (text == "+" || text == "-")
                        return ValidationStatus::PartialValid;
                      if (!isDigitsOnly(text)) return ValidationStatus::Invalid;

                      int val;
                      if (tryParseInt(text, val)) {
                        // Constraint: value >= 1 && value <= 1000
                        if (val >= 1 && val <= 1000)
                          return ValidationStatus::Valid;
                        return ValidationStatus::Invalid;
                      }

                      // If it's digits but doesn't fit in int, it's out of
                      // bounds (> 1000)
                      return ValidationStatus::Invalid;
                    },
                .compute_value = nullptr};

  /*
    field currency: String {
        value in ["USD", "EUR", "GBP"];
    }
  */
  fields["currency"] = FieldNode{
      .name = "currency",
      .type = FieldType::String,
      .is_deterministic = false,
      .validate =
          [](std::string_view text, const Environment&) {
            const std::vector<std::string> allowed = {"USD", "EUR", "GBP"};

            // Constraint: check if current text is an exact match
            if (std::ranges::any_of(
                    allowed, [&](const auto& opt) { return text == opt; }))
              return ValidationStatus::Valid;

            // Check if current text is a prefix of any allowed string
            if (std::ranges::any_of(allowed, [&](const auto& opt) {
                  return opt.rfind(text, 0) == 0;
                }))
              return ValidationStatus::PartialValid;

            return ValidationStatus::Invalid;
          },
      .compute_value = nullptr};

  /*
    field total_price: Number { }

    invariant valid_total_price {
        this.total_price == this.unit_price * this.quantity;
    }

    invariant bulk_discount {
        this.quantity > 500 -> this.total_price < (this.unit_price *
    this.quantity);
    }
  */
  fields["total_price"] = FieldNode{
      .name = "total_price",
      .type = FieldType::Number,
      .is_deterministic = true,
      .validate =
          [](std::string_view text, const Environment& env) {
            if (text.empty()) return ValidationStatus::PartialValid;
            if (!isValidDoublePrefix(text)) return ValidationStatus::Invalid;

            double val;
            if (tryParseDouble(text, val)) {
              // Fetch dependencies
              double unit_price = std::get<double>(env.at("unit_price"));
              int quantity = std::get<int>(env.at("quantity"));

              // Invariant 1
              double expected = unit_price * static_cast<double>(quantity);
              bool matches_total = (std::abs(val - expected) < 1e-5);

              // Invariant 2
              bool passes_bulk = true;
              if (quantity > 500) {
                passes_bulk = (val < expected);
              }

              if (matches_total && passes_bulk) {
                return ValidationStatus::Valid;
              }
              return ValidationStatus::Invalid;
            }
            return ValidationStatus::PartialValid;
          },
      // Solve for deterministic value
      .compute_value = [](const Environment& env) -> Value {
        double unit_price = std::get<double>(env.at("unit_price"));
        int quantity = std::get<int>(env.at("quantity"));
        return unit_price * static_cast<double>(quantity);
      }};
}

bool Runtime::hasMoreFields() const { return currStepIdx < genOrder.size(); }

std::string Runtime::getActiveFieldName() const {
  if (!hasMoreFields()) return "";
  return genOrder[currStepIdx];
}

FieldType Runtime::getActiveFieldType() const {
  if (!hasMoreFields()) throw std::runtime_error("No active field available.");
  return fields.at(getActiveFieldName()).type;
}

const std::vector<std::string>& Runtime::getGenOrder() const {
  return genOrder;
}

void Runtime::submitVal(std::string_view name, const Value& val) {
  std::string key{name};
  environment[key] = val;
  if (getActiveFieldName() == key) {
    currStepIdx++;
  }
}

void Runtime::submitValStr(std::string_view name, std::string_view raw_str) {
  std::string key{name};
  const auto& field = fields.at(key);

  if (field.type == FieldType::Integer) {
    int val;
    if (tryParseInt(raw_str, val)) {
      submitVal(name, Value(val));
    } else {
      throw std::runtime_error("Failed to parse Integer value: " +
                               std::string(raw_str));
    }
  } else if (field.type == FieldType::Number) {
    double val;
    if (tryParseDouble(raw_str, val)) {
      submitVal(name, Value(val));
    } else {
      throw std::runtime_error("Failed to parse Number value: " +
                               std::string(raw_str));
    }
  } else if (field.type == FieldType::String) {
    submitVal(name, Value(std::string(raw_str)));
  }
}

bool Runtime::isAciveFieldDeterministic() const {
  if (!hasMoreFields()) return false;
  return fields.at(getActiveFieldName()).is_deterministic;
}

ValidationStatus Runtime::validate_active_field_partial(
    std::string_view proposedChars) const {
  if (!hasMoreFields()) return ValidationStatus::Invalid;
  return fields.at(getActiveFieldName()).validate(proposedChars, environment);
}

const Environment& Runtime::get_environment() const { return environment; }

void Runtime::reset() {
  environment.clear();
  currStepIdx = 0;
}

std::string Runtime::solveDeterministic() {
  if (!hasMoreFields()) throw std::runtime_error("No active field to solve.");
  std::string name = getActiveFieldName();
  const auto& field = fields.at(name);

  if (!field.is_deterministic) {
    throw std::runtime_error("Field " + name + " is not deterministic.");
  }

  Value resolved_val = field.compute_value(environment);

  // Submit resolved value internally
  submitVal(name, resolved_val);

  // Convert to string to return to Python
  std::string str_rep;
  if (std::holds_alternative<int>(resolved_val)) {
    str_rep = std::to_string(std::get<int>(resolved_val));
  } else if (std::holds_alternative<double>(resolved_val)) {
    str_rep = std::to_string(std::get<double>(resolved_val));
  } else {
    str_rep = std::get<std::string>(resolved_val);
  }
  return str_rep;
}