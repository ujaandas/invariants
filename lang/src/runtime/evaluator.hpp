#pragma once

#include "bound_expr.hpp"
#include "types.hpp"

namespace invariants::runtime {

class Evaluator {
 private:
  mutable const Environment* currentEnv = nullptr;
  // When true, BinaryOp::In/Equal treat a string operand as an incomplete
  // prefix rather than a complete value, so in-progress LLM string
  // generation isn't rejected before it's finished.
  mutable bool partialMode = false;
  // Prefix to re-qualify a field access's flattenedPath against its
  // instantiation site (e.g. "profile.") before an environment lookup,
  // since nested specs are bound once and shared across instantiations.
  // Comes from InvariantTrigger::instancePrefix.
  mutable std::string instancePrefix;

 public:
  Value evaluate(const binder::BoundExpr& expr, const Environment& env,
                 bool partial = false,
                 const std::string& instancePrefix = "") const;

  Value operator()(const binder::BoundLiteralExpr& expr) const;
  Value operator()(const binder::BoundFieldAccessExpr& expr) const;
  Value operator()(const binder::BoundBinaryExpr& expr) const;
  Value operator()(const binder::BoundUnaryExpr& expr) const;
  Value operator()(const binder::BoundListExpr& expr) const;
  Value operator()(const binder::BoundValueAccessExpr& expr) const;
};

}  // namespace invariants::runtime