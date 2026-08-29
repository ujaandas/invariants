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
  std::vector<const binder::BoundInvariant*> allInv;

  for (const auto& spec : module.specs) {
    for (const auto& inv : spec.invariants) {
      allInv.push_back(&inv);

      auto deps = extractDeps(*inv.expression);

      if (inv.isDeterministicPossible && inv.target) {
        binder::FieldId target = inv.target->id;
        for (binder::FieldId src : deps) {
          if (src != target) {
            graph.addEdge(src, target);
          }
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

  for (const auto* inv : allInv) {
    auto deps = extractDeps(*inv->expression);

    if (deps.empty()) {
      if (!schedule.order.empty()) {
        schedule.triggers[schedule.order.front()].push_back(inv);
      }
      continue;
    }

    binder::FieldId latest_field = *deps.begin();
    std::size_t latest_pos = pos[latest_field];

    for (binder::FieldId dep : deps) {
      if (pos[dep] > latest_pos) {
        latest_pos = pos[dep];
        latest_field = dep;
      }
    }

    schedule.triggers[latest_field].push_back(inv);
  }

  return schedule;
}

}  // namespace invariants::analysis