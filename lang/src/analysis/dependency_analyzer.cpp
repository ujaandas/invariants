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

  for (const auto& spec : module.specs) {
    for (const auto& field : spec.fields) {
      binder::FieldId target_id =
          field.symbol
              ->id;  // Assuming BoundField holds a pointer to FieldSymbol
      for (const auto& constraint : field.constraints) {
        auto deps = extractDeps(*constraint.expr);
        for (binder::FieldId src : deps) {
          if (src != target_id) graph.addEdge(src, target_id);
        }
      }
    }

    for (const auto& inv : spec.invariants) {
      for (const auto& constraint : inv.constraints) {
        if (!constraint.isDeterministicPossible || !constraint.target) continue;

        auto deps = extractDeps(*constraint.expr);
        binder::FieldId target_id = constraint.target->id;
        for (binder::FieldId src : deps) {
          if (src != target_id) graph.addEdge(src, target_id);
        }
      }
    }
  }

  ExecutionSchedule schedule;
  schedule.order = graph.order();

  std::vector<std::size_t> pos(total_fields, 0);
  for (std::size_t idx = 0; idx < schedule.order.size(); ++idx) {
    pos[schedule.order[idx]] = idx;
  }

  // Helper lambda to DRY up the trigger scheduling logic
  auto scheduleTrigger = [&](const binder::BoundConstraint& constraint,
                             const binder::BoundInvariant* parentInv,
                             const std::unordered_set<binder::FieldId>& deps) {
    // Skip assignment constraints; they evaluate during field emission
    if (constraint.isDeterministicPossible && constraint.target) return;

    if (deps.empty()) {
      if (!schedule.order.empty()) {
        schedule.triggers[schedule.order.front()].push_back(
            {parentInv, &constraint});
      }
      return;
    }

    binder::FieldId latest_field = *deps.begin();
    std::size_t latest_pos = pos[latest_field];

    for (binder::FieldId dep : deps) {
      if (pos[dep] > latest_pos) {
        latest_pos = pos[dep];
        latest_field = dep;
      }
    }

    schedule.triggers[latest_field].push_back({parentInv, &constraint});
  };

  for (const auto& spec : module.specs) {
    // Schedule Field-level constraints
    for (const auto& field : spec.fields) {
      for (const auto& constraint : field.constraints) {
        auto deps = extractDeps(*constraint.expr);
        // Field constraints evaluate `value`, so they inherently depend on the
        // field they belong to!
        deps.insert(field.symbol->id);
        scheduleTrigger(constraint, nullptr, deps);
      }
    }

    // Schedule Invariant constraints
    for (const auto& inv : spec.invariants) {
      for (const auto& constraint : inv.constraints) {
        auto deps = extractDeps(*constraint.expr);
        scheduleTrigger(constraint, &inv, deps);
      }
    }
  }

  return schedule;
}

}  // namespace invariants::analysis