#include "evaluator.hpp"

namespace invariants::runtime {

Value Evaluator::evaluate(const binder::BoundExpr& expr,
                          const Environment& env) const {
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

  auto it = currentEnv->find(expr.flattenedPath);
  if (it == currentEnv->end()) {
    throw std::runtime_error("Field '" + expr.flattenedPath +
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

Value Evaluator::operator()(const binder::BoundUnaryExpr& expr) const {
  Value operand = std::visit(*this, expr.operand->value);

  if (expr.op == ast::UnaryOp::Not) {
    if (!std::holds_alternative<bool>(operand)) {
      throw std::runtime_error("Logical NOT (!) requires a Boolean operand.");
    }
    return !std::get<bool>(operand);
  }

  if (expr.op == ast::UnaryOp::Negate) {
    if (std::holds_alternative<double>(operand)) {
      return -std::get<double>(operand);
    } else if (std::holds_alternative<int>(operand)) {
      return -std::get<int>(operand);
    }
    throw std::runtime_error("Unary Minus (-) requires a numeric operand.");
  }

  throw std::runtime_error("Unknown unary operator.");
}

Value Evaluator::operator()(const binder::BoundBinaryExpr& expr) const {
  // Short-circuit logical ops
  if (expr.op == ast::BinaryOp::And || expr.op == ast::BinaryOp::Or ||
      expr.op == ast::BinaryOp::Imply) {
    Value leftVal = std::visit(*this, expr.left->value);
    bool l = std::get<bool>(leftVal);

    if (expr.op == ast::BinaryOp::And && !l) return false;
    if (expr.op == ast::BinaryOp::Or && l) return true;
    if (expr.op == ast::BinaryOp::Imply && !l)
      return true;  // False implies anything is True

    Value rightVal = std::visit(*this, expr.right->value);
    return std::get<bool>(rightVal);
  }

  // Evaluate both sides for math and relational operations
  Value leftVal = std::visit(*this, expr.left->value);
  Value rightVal = std::visit(*this, expr.right->value);

  // Helpers for numeric operations with auto-upcasting to double
  auto is_numeric = [](const Value& v) {
    return std::holds_alternative<int>(v) || std::holds_alternative<double>(v);
  };
  auto get_double = [](const Value& v) {
    return std::holds_alternative<double>(v)
               ? std::get<double>(v)
               : static_cast<double>(std::get<int>(v));
  };
  auto is_double = [](const Value& l, const Value& r) {
    return std::holds_alternative<double>(l) ||
           std::holds_alternative<double>(r);
  };

  switch (expr.op) {
    // Arithmetic
    case ast::BinaryOp::Add: {
      if (std::holds_alternative<std::string>(leftVal) &&
          std::holds_alternative<std::string>(rightVal)) {
        return std::get<std::string>(leftVal) + std::get<std::string>(rightVal);
      }
      if (is_double(leftVal, rightVal))
        return get_double(leftVal) + get_double(rightVal);
      return std::get<int>(leftVal) + std::get<int>(rightVal);
    }
    case ast::BinaryOp::Subtract: {
      if (is_double(leftVal, rightVal))
        return get_double(leftVal) - get_double(rightVal);
      return std::get<int>(leftVal) - std::get<int>(rightVal);
    }
    case ast::BinaryOp::Multiply: {
      if (is_double(leftVal, rightVal))
        return get_double(leftVal) * get_double(rightVal);
      return std::get<int>(leftVal) * std::get<int>(rightVal);
    }
    case ast::BinaryOp::Divide: {
      double r = get_double(rightVal);
      if (r == 0.0)
        throw std::runtime_error(
            "Division by zero encountered during evaluation.");
      if (is_double(leftVal, rightVal)) return get_double(leftVal) / r;
      return std::get<int>(leftVal) / std::get<int>(rightVal);
    }

    // Relational & equality
    case ast::BinaryOp::Equal: {
      if (is_numeric(leftVal) && is_numeric(rightVal))
        return get_double(leftVal) == get_double(rightVal);
      return leftVal ==
             rightVal;  // Fallback to std::variant equality for strings/bools
    }
    case ast::BinaryOp::NotEqual: {
      if (is_numeric(leftVal) && is_numeric(rightVal))
        return get_double(leftVal) != get_double(rightVal);
      return leftVal != rightVal;
    }
    case ast::BinaryOp::Greater:
      return get_double(leftVal) > get_double(rightVal);
    case ast::BinaryOp::Less:
      return get_double(leftVal) < get_double(rightVal);
    case ast::BinaryOp::GreaterEqual:
      return get_double(leftVal) >= get_double(rightVal);
    case ast::BinaryOp::LessEqual:
      return get_double(leftVal) <= get_double(rightVal);

    // List operations
    case ast::BinaryOp::In:
    case ast::BinaryOp::NotIn: {
      auto arrayPtr = std::get<std::shared_ptr<ArrayValue>>(rightVal);
      bool found = false;
      for (const auto& el : arrayPtr->elements) {
        if (is_numeric(leftVal) && is_numeric(el)) {
          if (get_double(leftVal) == get_double(el)) {
            found = true;
            break;
          }
        } else if (leftVal == el) {
          found = true;
          break;
        }
      }
      return expr.op == ast::BinaryOp::In ? found : !found;
    }

    default:
      throw std::runtime_error("Unknown binary operator.");
  }
}

}  // namespace invariants::runtime