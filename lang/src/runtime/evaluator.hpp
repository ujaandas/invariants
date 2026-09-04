#pragma once

#include "bound_expr.hpp"
#include "types.hpp"

namespace invariants::runtime {

class Evaluator {
 private:
  mutable const Environment* currentEnv = nullptr;
  // When true, BinaryOp::In treats a string operand as an incomplete/partial
  // value: membership is satisfied if it is a PREFIX of a list element,
  // rather than requiring exact equality. Used to validate in-progress LLM
  // string generation against IN constraints without rejecting valid prefixes.
  mutable bool partialMode = false;

 public:
  Value evaluate(const binder::BoundExpr& expr, const Environment& env,
                 bool partial = false) const;

  Value operator()(const binder::BoundLiteralExpr& expr) const;
  Value operator()(const binder::BoundFieldAccessExpr& expr) const;
  Value operator()(const binder::BoundBinaryExpr& expr) const;
  Value operator()(const binder::BoundUnaryExpr& expr) const;
  Value operator()(const binder::BoundListExpr& expr) const;
  Value operator()(const binder::BoundValueAccessExpr& expr) const;
};

}  // namespace invariants::runtime