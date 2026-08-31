#include "dependency_analyzer.hpp"

#include <algorithm>
#include <queue>
#include <stdexcept>
#include <variant>

#include "dependency_extractor.hpp"
#include "symbol_table.hpp"

namespace invariants::analysis {

std::unordered_set<std::string> DependencyAnalyzer::extractDeps(
    const binder::BoundExpr& expr, const std::string& prefix) {
  std::unordered_set<std::string> deps;
  DependencyExtractor extractor{deps, prefix};
  std::visit(extractor, expr.value);
  return deps;
}

void DependencyAnalyzer::unrollSpec(const binder::BoundModule& module,
                                    const binder::SpecSymbol* spec,
                                    const std::string& prefix) {
  for (const auto& boundSpec : module.specs) {
    if (boundSpec.symbol->name != spec->name) continue;

    for (const auto& field : boundSpec.fields) {
      std::string fullPath = prefix + field.symbol->name;

      if (field.symbol->resType.isBuiltin() ||
          field.symbol->resType.isArray() || field.symbol->resType.isMap()) {
        allNodes.push_back(fullPath);
      } else {
        const binder::SpecSymbol* nestedSpec =
            std::get<const binder::SpecSymbol*>(field.symbol->resType.type);
        unrollSpec(module, nestedSpec, fullPath + ".");
      }

      std::ranges::transform(
          field.constraints, std::back_inserter(allTriggers),
          [&prefix, &fullPath](const auto& constraint) -> InvariantTrigger {
            return {nullptr, &constraint, prefix, fullPath};
          });
    }

    for (const auto& inv : boundSpec.invariants) {
      std::ranges::transform(
          inv.constraints, std::back_inserter(allTriggers),
          [&inv, &prefix](const auto& constraint) -> InvariantTrigger {
            return {&inv, &constraint, prefix, ""};
          });
    }
  }
}

ExecutionSchedule DependencyAnalyzer::analyze(const binder::BoundModule& module,
                                              const std::string& rootSpecName) {
  allNodes.clear();
  allTriggers.clear();

  auto it = std::ranges::find_if(
      module.specs, [&](const auto& name) { return name == rootSpecName; },
      [](const auto& s) { return s.symbol->name; });

  const auto* rootSpec = (it != module.specs.end()) ? it->symbol : nullptr;

  if (!rootSpec) throw std::runtime_error("Root spec not found.");

  // Recursively extract all graph nodes and triggers
  unrollSpec(module, rootSpec, "");

  std::unordered_map<std::string, std::vector<std::string>> adj;
  std::unordered_map<std::string, int> inDegree;
  for (const auto& node : allNodes) inDegree[node] = 0;

  // Build Adjacency List
  for (const auto& trigger : allTriggers) {
    if (trigger.constraint->isDeterministicPossible) {
      std::string target = trigger.instancePrefix + trigger.constraint->target;
      auto deps =
          extractDeps(*trigger.constraint->expr, trigger.instancePrefix);

      for (const auto& dep : deps) {
        if (dep != target) {
          adj[dep].push_back(target);
          inDegree[target]++;
        }
      }
    }
  }

  // Execute Kahn's Topological Sort
  ExecutionSchedule schedule;
  std::queue<std::string> q;
  for (const auto& [node, degree] : inDegree) {
    if (degree == 0) q.push(node);
  }

  while (!q.empty()) {
    std::string curr = q.front();
    q.pop();
    schedule.order.push_back(curr);

    for (const auto& neighbor : adj[curr]) {
      if (--inDegree[neighbor] == 0) q.push(neighbor);
    }
  }

  if (schedule.order.size() != allNodes.size()) {
    throw std::runtime_error("Cycle detected in assignment dependencies.");
  }

  // Attach Triggers to the latest generated dependency
  for (const auto& trigger : allTriggers) {
    if (trigger.constraint->isDeterministicPossible) {
      std::string target = trigger.instancePrefix + trigger.constraint->target;
      schedule.triggers[target].push_back(trigger);
    } else {
      auto deps =
          extractDeps(*trigger.constraint->expr, trigger.instancePrefix);
      if (!trigger.ownerFieldPath.empty()) deps.insert(trigger.ownerFieldPath);

      std::string latest_dep =
          deps.empty() ? schedule.order.front() : *deps.begin();
      int latest_idx = -1;

      for (const auto& dep : deps) {
        auto dep_it =
            std::find(schedule.order.begin(), schedule.order.end(), dep);
        if (dep_it != schedule.order.end()) {
          int idx = std::distance(schedule.order.begin(), dep_it);
          if (idx > latest_idx) {
            latest_idx = idx;
            latest_dep = dep;
          }
        }
      }
      schedule.triggers[latest_dep].push_back(trigger);
    }
  }

  return schedule;
}

}  // namespace invariants::analysis