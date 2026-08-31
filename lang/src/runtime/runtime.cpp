#include "runtime.hpp"

#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <variant>

namespace invariants::runtime {

static std::vector<std::string> splitPath(const std::string& s) {
  std::vector<std::string> parts;
  std::istringstream stream(s);
  std::string part;
  while (std::getline(stream, part, '.')) {
    parts.push_back(part);
  }
  return parts;
}

static Value parseLLMString(std::string_view raw,
                            const binder::ResolvedType& type) {
  if (!type.isBuiltin()) {
    throw std::runtime_error(
        "Nested object parsing from LLM not supported directly.");
  }

  ast::BuiltinType builtin = std::get<ast::BuiltinType>(type.type);
  std::string s(raw);

  try {
    switch (builtin) {
      case ast::BuiltinType::String:
        if (s.size() >= 2 && s.front() == '"' && s.back() == '"') {
          return s.substr(1, s.size() - 2);
        }
        return s;
      case ast::BuiltinType::Integer:
        return std::stoi(s);
      case ast::BuiltinType::Number:
        return std::stod(s);
      case ast::BuiltinType::Boolean:
        return (s == "true" || s == "True");
      default:
        return std::monostate{};
    }
  } catch (...) {
    throw std::runtime_error("Failed to cast LLM string '" + s +
                             "' to expected type.");
  }
}

static std::string valueToString(const Value& val) {
  if (std::holds_alternative<int>(val))
    return std::to_string(std::get<int>(val));
  if (std::holds_alternative<double>(val))
    return std::to_string(std::get<double>(val));
  if (std::holds_alternative<bool>(val))
    return std::get<bool>(val) ? "true" : "false";
  if (std::holds_alternative<std::string>(val))
    return std::get<std::string>(val);
  return "null";
}

Runtime::Runtime(const binder::BoundModule& module,
                 const analysis::ExecutionSchedule& schedule)
    : boundModule(module), schedule(schedule) {
  reset();
}

void Runtime::reset() {
  environment.clear();
  currStepIdx = 0;
}

bool Runtime::hasMoreFields() const {
  return currStepIdx < schedule.order.size();
}

std::string Runtime::getActiveFieldName() const {
  if (!hasMoreFields()) throw std::runtime_error("Generation complete.");
  return schedule.order[currStepIdx];
}

const binder::FieldSymbol* Runtime::getActiveFieldSymbol() const {
  std::string activePath = getActiveFieldName();
  auto parts = splitPath(activePath);

  // Try treating each spec as the root spec until one successfully resolves the
  // full path
  for (const auto& potentialRoot : boundModule.specs) {
    const binder::SpecSymbol* currentSpec = potentialRoot.symbol;
    const binder::FieldSymbol* finalField = nullptr;
    bool pathResolved = true;

    for (size_t i = 0; i < parts.size(); ++i) {
      bool found = false;

      // Find the bound spec definition for our current level
      auto specIt = std::ranges::find_if(boundModule.specs, [&](const auto& s) {
        return s.symbol->name == currentSpec->name;
      });

      if (specIt != boundModule.specs.end()) {
        auto fieldIt = std::ranges::find_if(specIt->fields, [&](const auto& f) {
          return f.symbol->name == parts[i];
        });

        if (fieldIt != specIt->fields.end()) {
          finalField = fieldIt->symbol;
          found = true;

          // If we have more parts to resolve, advance the current spec to the
          // nested type
          if (i < parts.size() - 1) {
            currentSpec =
                std::get<const binder::SpecSymbol*>(finalField->resType.type);
          }
        }
      }

      if (!found) {
        pathResolved = false;
        break;  // Path failed under this root candidate; break out to try the
                // next spec
      }
    }

    if (pathResolved && finalField) {
      return finalField;  // Path fully resolved!
    }
  }

  throw std::runtime_error("Could not resolve symbol for path: " + activePath);
}

bool Runtime::isActiveFieldDeterministic() const {
  if (!hasMoreFields()) return false;
  std::string activePath = getActiveFieldName();

  auto it = schedule.triggers.find(activePath);
  if (it != schedule.triggers.end()) {
    return std::ranges::any_of(it->second, [&activePath](const auto& trigger) {
      return trigger.constraint->isDeterministicPossible &&
             (trigger.constraint->target == activePath ||
              trigger.constraint->target.empty());
    });
  }
  return false;
}

std::string Runtime::solveDeterministic() {
  if (!isActiveFieldDeterministic()) {
    throw std::runtime_error("Active field is not deterministic.");
  }

  std::string activePath = getActiveFieldName();
  const binder::BoundConstraint* assignmentConstraint = nullptr;

  const auto& triggers = schedule.triggers.at(activePath);
  auto it = std::ranges::find_if(triggers, [&activePath](const auto& trigger) {
    return trigger.constraint->isDeterministicPossible &&
           (trigger.constraint->target == activePath ||
            trigger.constraint->target.empty());
  });

  if (it != triggers.end()) {
    assignmentConstraint = it->constraint;
  }

  if (!assignmentConstraint) {
    throw std::runtime_error(
        "Fatal: Could not find assignment constraint for deterministic field.");
  }

  const auto& equalityExpr =
      std::get<binder::BoundBinaryExpr>(assignmentConstraint->expr->value);

  const binder::BoundExpr* calcExpr = nullptr;

  // Check if the Left side is our target
  if (std::holds_alternative<binder::BoundValueAccessExpr>(
          equalityExpr.left->value)) {
    calcExpr = equalityExpr.right.get();
  } else if (std::holds_alternative<binder::BoundFieldAccessExpr>(
                 equalityExpr.left->value) &&
             std::get<binder::BoundFieldAccessExpr>(equalityExpr.left->value)
                     .flattenedPath == activePath) {
    calcExpr = equalityExpr.right.get();
  }
  // Check if the Right side is our target
  else if (std::holds_alternative<binder::BoundValueAccessExpr>(
               equalityExpr.right->value)) {
    calcExpr = equalityExpr.left.get();
  } else if (std::holds_alternative<binder::BoundFieldAccessExpr>(
                 equalityExpr.right->value) &&
             std::get<binder::BoundFieldAccessExpr>(equalityExpr.right->value)
                     .flattenedPath == activePath) {
    calcExpr = equalityExpr.left.get();
  } else {
    throw std::runtime_error(
        "Fatal: Could not locate assignment target in expression.");
  }

  Value computedValue = evaluator.evaluate(*calcExpr, environment);

  submitVal(activePath, computedValue);
  return valueToString(computedValue);
}

ValidationStatus Runtime::validatePartial(
    std::string_view proposedChars) const {
  if (!hasMoreFields()) return ValidationStatus::Invalid;

  std::string activePath = getActiveFieldName();
  const auto* activeField = getActiveFieldSymbol();

  Value proposedVal;
  try {
    proposedVal = parseLLMString(proposedChars, activeField->resType);
  } catch (...) {
    return ValidationStatus::PartialValid;
  }

  Environment tempEnv = environment;
  tempEnv["__value__"] = proposedVal;
  tempEnv[activePath] = proposedVal;

  auto it = schedule.triggers.find(activePath);
  if (it != schedule.triggers.end()) {
    for (const auto& trigger : it->second) {
      if (trigger.constraint->isDeterministicPossible) continue;

      try {
        Value res = evaluator.evaluate(*trigger.constraint->expr, tempEnv);
        if (std::holds_alternative<bool>(res) && !std::get<bool>(res)) {
          return ValidationStatus::Invalid;
        }
      } catch (...) {
        return ValidationStatus::Invalid;
      }
    }
  }

  return ValidationStatus::Valid;
}

void Runtime::submitValStr(std::string_view name, std::string_view raw_str) {
  if (name != getActiveFieldName()) {
    throw std::runtime_error(
        "Attempted to submit value for an inactive field.");
  }

  const auto* activeField = getActiveFieldSymbol();
  Value finalVal = parseLLMString(raw_str, activeField->resType);
  submitVal(std::string(name), finalVal);
}

void Runtime::submitVal(const std::string& name, const Value& val) {
  environment[name] = val;
  currStepIdx++;
}

const Environment& Runtime::getEnvironment() const { return environment; }

}  // namespace invariants::runtime