#pragma once

#include "./bound_expr.hpp"

namespace invariants::binder {

// Local constraint inside invariant
struct BoundConstraint {
  BoundExprPtr expr;

  // Assignment metadata (populated if root is `this.x == ...`)
  bool isDeterministicPossible = false;
  const FieldSymbol* target = nullptr;
};

// Cross-field invariant
struct BoundInvariant {
  std::string name;
  std::vector<BoundConstraint> constraints;
};

// A fully bound field
struct BoundField {
  const FieldSymbol* symbol = nullptr;
  std::vector<BoundConstraint> constraints;
};

// A fully bound spec
struct BoundSpec {
  const SpecSymbol* symbol = nullptr;
  std::vector<BoundField> fields;
  std::vector<BoundInvariant> invariants;
};

// The root of the bound tree
struct BoundModule {
  std::vector<BoundSpec> specs;
};

}  // namespace invariants::binder