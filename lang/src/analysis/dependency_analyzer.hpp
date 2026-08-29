#pragma once

#include <unordered_set>

#include "bound_expr.hpp"
#include "bound_stmt.hpp"
#include "symbol_table.hpp"

namespace invariants::analysis {

struct ExecutionSchedule {
  std::vector<binder::FieldId> order;
  std::unordered_map<binder::FieldId,
                     std::vector<const binder::BoundInvariant*>>
      triggers;
};

class DependencyAnalyzer {
 public:
  ExecutionSchedule analyze(const binder::BoundModule& module,
                            std::size_t total_fields);

  static std::unordered_set<binder::FieldId> extractDeps(
      const binder::BoundExpr& expr);
};

}  // namespace invariants::analysis