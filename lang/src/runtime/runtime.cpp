#include "runtime.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <variant>

namespace invariants::runtime {

namespace {

// True if `expr` refers to the value currently being generated -- either the
// `value` keyword, or a field access matching `activePath`. `instancePrefix`
// re-qualifies a nested spec's own flattenedPath (e.g. "vcpu_cores") against
// its instantiation site (e.g. "profile.vcpu_cores").
bool isActiveValueRef(const binder::BoundExpr& expr, const std::string& activePath,
                      const std::string& instancePrefix) {
  if (std::holds_alternative<binder::BoundValueAccessExpr>(expr.value)) return true;
  if (std::holds_alternative<binder::BoundFieldAccessExpr>(expr.value)) {
    return (instancePrefix +
            std::get<binder::BoundFieldAccessExpr>(expr.value).flattenedPath) ==
           activePath;
  }
  return false;
}

double asDouble(const Value& v) {
  if (std::holds_alternative<int>(v)) return static_cast<double>(std::get<int>(v));
  return std::get<double>(v);
}

// Evaluates `expr` as a plain constant using already-committed field values,
// e.g. the `this.vcpu_cores * 2.0` side of `this.ram_gb >= this.vcpu_cores *
// 2.0`. Returns nullopt if it isn't numeric or can't be evaluated yet.
std::optional<double> tryEvaluateConstant(const binder::BoundExpr& expr,
                                          const Environment& env,
                                          const std::string& instancePrefix,
                                          const Evaluator& evaluator) {
  try {
    Value v = evaluator.evaluate(expr, env, /*partial=*/false, instancePrefix);
    if (std::holds_alternative<int>(v)) return static_cast<double>(std::get<int>(v));
    if (std::holds_alternative<double>(v)) return std::get<double>(v);
  } catch (...) {
  }
  return std::nullopt;
}

// Recognizes `this.field <op> expr` (or reversed) and normalizes it to
// "value <op> k", regardless of which side the active field was on.
std::optional<std::pair<ast::BinaryOp, double>> asNormalizedThreshold(
    const binder::BoundExpr& expr, const std::string& activePath,
    const std::string& instancePrefix, const Environment& env,
    const Evaluator& evaluator) {
  if (!std::holds_alternative<binder::BoundBinaryExpr>(expr.value))
    return std::nullopt;
  const auto& bin = std::get<binder::BoundBinaryExpr>(expr.value);

  if (bin.op != ast::BinaryOp::Less && bin.op != ast::BinaryOp::LessEqual &&
      bin.op != ast::BinaryOp::Greater && bin.op != ast::BinaryOp::GreaterEqual) {
    return std::nullopt;
  }

  bool valueOnLeft = isActiveValueRef(*bin.left, activePath, instancePrefix);
  bool valueOnRight = isActiveValueRef(*bin.right, activePath, instancePrefix);
  if (valueOnLeft == valueOnRight) return std::nullopt;

  std::optional<double> k =
      valueOnLeft ? tryEvaluateConstant(*bin.right, env, instancePrefix, evaluator)
                  : tryEvaluateConstant(*bin.left, env, instancePrefix, evaluator);
  if (!k.has_value()) return std::nullopt;

  ast::BinaryOp op = bin.op;
  if (!valueOnLeft) {
    switch (op) {
      case ast::BinaryOp::Less: op = ast::BinaryOp::Greater; break;
      case ast::BinaryOp::LessEqual: op = ast::BinaryOp::GreaterEqual; break;
      case ast::BinaryOp::Greater: op = ast::BinaryOp::Less; break;
      case ast::BinaryOp::GreaterEqual: op = ast::BinaryOp::LessEqual; break;
      default: return std::nullopt;
    }
  }

  return std::make_pair(op, *k);
}

// Proves a numeric threshold constraint is already unsatisfiable by a
// still-growing prefix, without waiting for the value to be complete. More
// digits can only grow a number's magnitude, so the current prefix is a
// sound lower bound (if >= 0) or upper bound (if < 0) on the final value.
// Returns false if provably violated, true if not (yet) violated, nullopt if
// `expr` isn't a recognized threshold shape.
std::optional<bool> tryPruneNumericRange(const binder::BoundExpr& expr,
                                         const std::string& activePath,
                                         const std::string& instancePrefix,
                                         const Value& proposedVal,
                                         std::string_view proposedChars,
                                         const Environment& env,
                                         const Evaluator& evaluator) {
  if (!std::holds_alternative<int>(proposedVal) &&
      !std::holds_alternative<double>(proposedVal)) {
    return std::nullopt;
  }
  auto normalized = asNormalizedThreshold(expr, activePath, instancePrefix, env, evaluator);
  if (!normalized.has_value()) return std::nullopt;
  auto [op, k] = *normalized;
  double v = asDouble(proposedVal);

  if (v >= 0) {
    if (op == ast::BinaryOp::Less && v >= k) return false;
    if (op == ast::BinaryOp::LessEqual && v > k) return false;
  } else {
    if (op == ast::BinaryOp::Greater && v <= k) return false;
    if (op == ast::BinaryOp::GreaterEqual && v < k) return false;
  }

  // Opposite-direction bound: once no more integer digits can follow (a
  // decimal point was typed, or the integer part is a lone "0", which JSON
  // grammar forbids extending), the value is confined to a window around v
  // no wider than 10^-(fractional digits already typed) -- e.g. "0.005" (3
  // fractional digits) can only ever land in [0.005, 0.006), regardless of
  // what follows, since appending more digits can't touch the ones already
  // fixed. Without any fractional digits yet (e.g. a lone "0"), the window
  // is the full unit interval, same as before.
  bool hasDecimalPoint = proposedChars.find('.') != std::string_view::npos;
  bool hasExponent = proposedChars.find('e') != std::string_view::npos ||
                     proposedChars.find('E') != std::string_view::npos;
  bool intPartIsLoneZero = false;
  {
    std::string_view s = proposedChars;
    if (!s.empty() && s.front() == '-') s.remove_prefix(1);
    size_t stop = s.find_first_of(".eE");
    std::string_view intPart = (stop == std::string_view::npos) ? s : s.substr(0, stop);
    intPartIsLoneZero = (intPart == "0");
  }
  if ((hasDecimalPoint || intPartIsLoneZero) && !hasExponent) {
    double windowWidth = 1.0;
    if (hasDecimalPoint) {
      size_t dotPos = proposedChars.find('.');
      size_t fracDigits = proposedChars.size() - dotPos - 1;
      windowWidth = std::pow(10.0, -static_cast<double>(fracDigits));
    }
    if (v >= 0) {
      // upperBound is a strict supremum -- the value can get arbitrarily
      // close to it but never reach it -- so k == upperBound is just as
      // unreachable as k > upperBound for both > and >=.
      double upperBound = v + windowWidth;
      if (op == ast::BinaryOp::Greater && k >= upperBound) return false;
      if (op == ast::BinaryOp::GreaterEqual && k >= upperBound) return false;
    } else {
      double lowerBound = v - windowWidth;
      if (op == ast::BinaryOp::Less && k <= lowerBound) return false;
      if (op == ast::BinaryOp::LessEqual && k <= lowerBound) return false;
    }
  }

  return true;
}

// A lone '-' hasn't parsed as a number yet, but the eventual value is
// already known to be <= 0. If a threshold constraint rules out every
// non-positive value, the sign alone is a dead end.
bool isDeadNegativeSign(const binder::BoundExpr& expr, const std::string& activePath,
                        const std::string& instancePrefix, const Environment& env,
                        const Evaluator& evaluator) {
  auto normalized = asNormalizedThreshold(expr, activePath, instancePrefix, env, evaluator);
  if (!normalized.has_value()) return false;
  auto [op, k] = *normalized;
  if (op == ast::BinaryOp::Greater && k >= 0.0) return true;
  if (op == ast::BinaryOp::GreaterEqual && k > 0.0) return true;
  return false;
}

}  // namespace

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
  if (std::holds_alternative<double>(val)) {
    // Strip std::to_string's fixed 6-decimal padding, keeping one digit
    // after the '.' so it still reads as a Number, not an Integer.
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(6) << std::get<double>(val);
    std::string s = oss.str();
    size_t last_nonzero = s.find_last_not_of('0');
    if (s[last_nonzero] == '.') last_nonzero++;
    s.erase(last_nonzero + 1);
    return s;
  }
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
      // Mirror DependencyAnalyzer's own target computation: a nested-spec
      // assignment's target is relative to that spec and needs
      // instancePrefix re-applied; an empty target falls back to
      // ownerFieldPath, which is already fully qualified.
      std::string target = trigger.constraint->target.empty()
                                ? trigger.ownerFieldPath
                                : trigger.instancePrefix + trigger.constraint->target;
      return trigger.constraint->isDeterministicPossible && target == activePath;
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
  std::string instancePrefix;

  const auto& triggers = schedule.triggers.at(activePath);
  auto it = std::ranges::find_if(triggers, [&activePath](const auto& trigger) {
    std::string target = trigger.constraint->target.empty()
                              ? trigger.ownerFieldPath
                              : trigger.instancePrefix + trigger.constraint->target;
    return trigger.constraint->isDeterministicPossible && target == activePath;
  });

  if (it != triggers.end()) {
    assignmentConstraint = it->constraint;
    instancePrefix = it->instancePrefix;
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
             (instancePrefix + std::get<binder::BoundFieldAccessExpr>(
                                    equalityExpr.left->value)
                                    .flattenedPath) == activePath) {
    calcExpr = equalityExpr.right.get();
  }
  // Check if the Right side is our target
  else if (std::holds_alternative<binder::BoundValueAccessExpr>(
               equalityExpr.right->value)) {
    calcExpr = equalityExpr.left.get();
  } else if (std::holds_alternative<binder::BoundFieldAccessExpr>(
                 equalityExpr.right->value) &&
             (instancePrefix + std::get<binder::BoundFieldAccessExpr>(
                                    equalityExpr.right->value)
                                    .flattenedPath) == activePath) {
    calcExpr = equalityExpr.left.get();
  } else {
    throw std::runtime_error(
        "Fatal: Could not locate assignment target in expression.");
  }

  Value computedValue =
      evaluator.evaluate(*calcExpr, environment, /*partial=*/false, instancePrefix);

  submitVal(activePath, computedValue);
  return valueToString(computedValue);
}

ValidationStatus Runtime::validatePartial(std::string_view proposedChars,
                                          bool isComplete) const {
  if (!hasMoreFields()) return ValidationStatus::Invalid;

  std::string activePath = getActiveFieldName();
  const auto* activeField = getActiveFieldSymbol();

  Value proposedVal;
  try {
    proposedVal = parseLLMString(proposedChars, activeField->resType);
  } catch (...) {
    // A lone '-' fails to parse (no digits yet); prune it if the field's
    // bounds already rule out every non-positive value.
    if (!isComplete && proposedChars == "-" && activeField->resType.isBuiltin() &&
        (std::get<ast::BuiltinType>(activeField->resType.type) ==
             ast::BuiltinType::Integer ||
         std::get<ast::BuiltinType>(activeField->resType.type) ==
             ast::BuiltinType::Number)) {
      auto negIt = schedule.triggers.find(activePath);
      if (negIt != schedule.triggers.end()) {
        for (const auto& trigger : negIt->second) {
          if (trigger.constraint->isDeterministicPossible) continue;
          if (isDeadNegativeSign(*trigger.constraint->expr, activePath,
                                 trigger.instancePrefix, environment, evaluator)) {
            return ValidationStatus::Invalid;
          }
        }
      }
    }
    return ValidationStatus::PartialValid;
  }

  Environment tempEnv = environment;
  tempEnv["__value__"] = proposedVal;
  tempEnv[activePath] = proposedVal;

  auto it = schedule.triggers.find(activePath);
  if (it != schedule.triggers.end()) {
    for (const auto& trigger : it->second) {
      if (trigger.constraint->isDeterministicPossible) continue;

      if (!isComplete) {
        auto pruned = tryPruneNumericRange(*trigger.constraint->expr, activePath,
                                           trigger.instancePrefix, proposedVal,
                                           proposedChars, tempEnv, evaluator);
        if (pruned.has_value()) {
          if (!*pruned) return ValidationStatus::Invalid;
          // Not yet provably violated -- skip the plain comparison below,
          // which isn't partial-safe.
          continue;
        }
      }

      try {
        Value res = evaluator.evaluate(*trigger.constraint->expr, tempEnv,
                                       /*partial=*/!isComplete,
                                       trigger.instancePrefix);
        if (std::holds_alternative<bool>(res) && !std::get<bool>(res)) {
          return ValidationStatus::Invalid;
        }
      } catch (...) {
        return ValidationStatus::Invalid;
      }
    }
  }

  return isComplete ? ValidationStatus::Valid : ValidationStatus::PartialValid;
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
