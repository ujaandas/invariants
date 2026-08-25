#pragma once

#include "./bound_expr.hpp"

namespace invariants::binder {

// Local constraint inside invariant
struct BoundConstraint {
  BoundExprPtr expr;
};

// Cross-field invariant
struct BoundInvariant {
  std::string name;
  BoundExprPtr expression;  // The logic

  bool isDeterministicPossible = false;
  const FieldSymbol* target = nullptr;  // Set if this computes a value
};

// A fully bound field
struct BoundField {
  const FieldSymbol* symbol;
  std::vector<BoundConstraint> local_constraints;
};

// A fully bound spec
struct BoundSpec {
  const SpecSymbol* symbol;
  std::vector<BoundField> fields;
  std::vector<BoundInvariant> invariants;
};

// The root of the bound tree
struct BoundModule {
  std::vector<BoundSpec> specs;
};

}  // namespace invariants::binder