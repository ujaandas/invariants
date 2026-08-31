#pragma once

#include <string>
#include <unordered_set>

#include "bound_expr.hpp"

namespace invariants::analysis {

struct DependencyExtractor {
  std::unordered_set<std::string>& deps;
  std::string prefix;

  void operator()(const binder::BoundLiteralExpr&) const;
  void operator()(const binder::BoundValueAccessExpr&) const;
  void operator()(const binder::BoundFieldAccessExpr&) const;
  void operator()(const binder::BoundUnaryExpr&) const;
  void operator()(const binder::BoundBinaryExpr&) const;
  void operator()(const binder::BoundListExpr&) const;
};

}  // namespace invariants::analysis