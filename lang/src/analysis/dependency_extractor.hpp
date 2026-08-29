#pragma once

#include <unordered_set>

#include "bound_expr.hpp"
#include "symbol_table.hpp"

namespace invariants::analysis {

struct DependencyExtractor {
  std::unordered_set<binder::FieldId>& deps;

  void operator()(const binder::BoundLiteralExpr&) const {};
  void operator()(const binder::BoundValueAccessExpr&) const {};
  void operator()(const binder::BoundFieldAccessExpr&) const {};
  void operator()(const binder::BoundUnaryExpr&) const {};
  void operator()(const binder::BoundBinaryExpr&) const {};
};

}  // namespace invariants::analysis