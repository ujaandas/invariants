#include "evaluator.hpp"

namespace invariants::runtime {

Value Evaluator::evaluate(const binder::BoundExpr& expr,
                          const Environment& env) {
  // Store env for this pass
  currentEnv = &env;
  return std::visit(*this, expr.value);
}

Value Evaluator::operator()(const binder::BoundLiteralExpr& expr) const {
  // Translate binder's literal variants into our value variants
  return std::visit(
      [](auto&& arg) -> Value {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, bool> || std::is_same_v<T, int> ||
                      std::is_same_v<T, double> ||
                      std::is_same_v<T, std::string>) {
          return arg;
        } else {
          return std::monostate{};
        }
      },
      expr.value);
}

Value Evaluator::operator()(const binder::BoundFieldAccessExpr& expr) const {
  if (!currentEnv) throw std::runtime_error("Evaluator environment is null.");

  auto it = currentEnv->find(expr.field->name);
  if (it == currentEnv->end()) {
    throw std::runtime_error("Field '" + expr.field->name +
                             "' not found in generation environment.");
  }

  return it->second;
}

Value Evaluator::operator()(const binder::BoundValueAccessExpr& expr) const {
  if (!currentEnv) throw std::runtime_error("Evaluator environment is null.");

  // 'value' keyword refers to the active field currently being generated
  // Will inject it into the map as a special key
  auto it = currentEnv->find("__value__");
  if (it == currentEnv->end()) {
    throw std::runtime_error(
        "Contextual '__value__' not found in environment.");
  }

  return it->second;
}

Value Evaluator::operator()(const binder::BoundListExpr& expr) const {
  auto arrayVal = std::make_shared<ArrayValue>();
  for (const auto& el : expr.elements) {
    if (el) {
      arrayVal->elements.push_back(std::visit(*this, el->value));
    }
  }
  return arrayVal;
}

}  // namespace invariants::runtime