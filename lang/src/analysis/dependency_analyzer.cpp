#include "dependency_analyzer.hpp"

#include <unordered_set>
#include <variant>

#include "dependency_extractor.hpp"
#include "dependency_graph.hpp"
#include "symbol_table.hpp"

namespace invariants::analysis {

std::unordered_set<binder::FieldId> DependencyAnalyzer::extractDeps(
    const binder::BoundExpr& expr) {
  std::unordered_set<binder::FieldId> deps;
  DependencyExtractor extractor{deps};
  std::visit(extractor, expr.value);
  return deps;
}

ExecutionSchedule DependencyAnalyzer::analyze(const binder::BoundModule& module,
                                              std::size_t total_fields) {
  DependencyGraph graph(total_fields);

  // Build dependency graph from assignment constraints
  for (const auto& spec : module.specs) {
    for (const auto& inv : spec.invariants) {
      for (const auto& constraint : inv.constraints) {
        // Deterministic assignments are evaluated at target generation time,
        // so we only schedule validation triggers for pure checks
        if (!constraint.isDeterministicPossible || !constraint.target) {
          continue;
        }

        auto deps = extractDeps(*constraint.expr);
        binder::FieldId target_id = constraint.target->id;

        for (binder::FieldId src : deps) {
          if (src != target_id) {
            graph.addEdge(src, target_id);
          }
        }
      }
    }
  }

  // Compute topological field generation order
  ExecutionSchedule schedule;
  schedule.order = graph.order();

  // Precompute field position in the topological sequence for O(1) lookups
  std::vector<std::size_t> pos(total_fields, 0);
  for (std::size_t idx = 0; idx < schedule.order.size(); ++idx) {
    pos[schedule.order[idx]] = idx;
  }

  // Schedule validation triggers for non-assignment constraints
  for (const auto& spec : module.specs) {
    for (const auto& inv : spec.invariants) {
      for (const auto& constraint : inv.constraints) {
        // Skip assignment constraints; they evaluate during field emission
        if (constraint.isDeterministicPossible && constraint.target) {
          continue;
        }

        auto deps = extractDeps(*constraint.expr);

        // Constant or parameterless checks trigger at the start of generation
        if (deps.empty()) {
          if (!schedule.order.empty()) {
            schedule.triggers[schedule.order.front()].push_back(
                {&inv, &constraint});
          }
          continue;
        }

        // Schedule check on the latest dependency evaluated in the topological
        // order
        binder::FieldId latest_field = *deps.begin();
        std::size_t latest_pos = pos[latest_field];

        for (binder::FieldId dep : deps) {
          if (pos[dep] > latest_pos) {
            latest_pos = pos[dep];
            latest_field = dep;
          }
        }

        schedule.triggers[latest_field].push_back({&inv, &constraint});
      }
    }
  }

  return schedule;
}

}  // namespace invariants::analysis