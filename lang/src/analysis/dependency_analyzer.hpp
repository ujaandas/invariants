#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "bound_expr.hpp"
#include "bound_stmt.hpp"
#include "symbol_table.hpp"

namespace invariants::analysis {

struct InvariantTrigger {
  const binder::BoundInvariant* parentInv;
  const binder::BoundConstraint* constraint;
  std::string instancePrefix;
  std::string ownerFieldPath;
};

struct ExecutionSchedule {
  std::vector<std::string> order;

  // Maps a flattened field path to all constraint checks triggered
  std::unordered_map<std::string, std::vector<InvariantTrigger>> triggers;
};

class DependencyAnalyzer {
 public:
  ExecutionSchedule analyze(const binder::BoundModule& module,
                            const std::string& rootSpecName);

  static std::unordered_set<std::string> extractDeps(
      const binder::BoundExpr& expr, const std::string& prefix);

 private:
  std::vector<std::string> allNodes;
  std::vector<InvariantTrigger> allTriggers;

  void unrollSpec(const binder::BoundModule& module,
                  const binder::SpecSymbol* spec, const std::string& prefix);
};

}  // namespace invariants::analysis