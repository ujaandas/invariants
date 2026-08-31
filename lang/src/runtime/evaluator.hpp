#pragma once

#include "bound_expr.hpp"
#include "types.hpp"

namespace invariants::runtime {

class Evaluator {
 private:
  mutable const Environment* currentEnv = nullptr;

 public:
  Value evaluate(const binder::BoundExpr& expr, const Environment& env) const;

  Value operator()(const binder::BoundLiteralExpr& expr) const;
  Value operator()(const binder::BoundFieldAccessExpr& expr) const;
  Value operator()(const binder::BoundBinaryExpr& expr) const;
  Value operator()(const binder::BoundUnaryExpr& expr) const;
  Value operator()(const binder::BoundListExpr& expr) const;
  Value operator()(const binder::BoundValueAccessExpr& expr) const;
};

}  // namespace invariants::runtime