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
      std::string target =
          trigger.constraint->target.empty()
              ? trigger.ownerFieldPath
              : trigger.instancePrefix + trigger.constraint->target;
      auto deps =
          extractDeps(*trigger.constraint->expr, trigger.instancePrefix);

      for (const auto& dep : deps) {
        if (dep != target) {
          adj[dep].push_back(target);
          inDegree[target]++;
        }
      }
    } else if (!trigger.ownerFieldPath.empty()) {
      // A field's own inline validation constraint (e.g. `field b: Integer {
      // value >= this.a; }`) must also schedule after whatever other fields
      // it references -- otherwise, if one of those fields ends up later in
      // generation order anyway, trigger attachment below binds the
      // constraint to that later field instead of to its owner, and it
      // never actually gets checked against b.
      auto deps =
          extractDeps(*trigger.constraint->expr, trigger.instancePrefix);
      for (const auto& dep : deps) {
        if (dep != trigger.ownerFieldPath) {
          adj[dep].push_back(trigger.ownerFieldPath);
          inDegree[trigger.ownerFieldPath]++;
        }
      }
    } else if (std::holds_alternative<binder::BoundBinaryExpr>(
                   trigger.constraint->expr->value) &&
               std::get<binder::BoundBinaryExpr>(trigger.constraint->expr->value)
                       .op == ast::BinaryOp::Imply) {
      // A standalone `invariant { }` block has no owning field of its own
      // (ownerFieldPath is always empty for these), so a general validation
      // constraint referencing multiple fields has no principled "check
      // this one last" target -- which field a boolean expression is
      // really validating isn't recoverable from its syntax in general.
      // An implication is the one shape where it IS recoverable: `A -> B`
      // reads as "given A, B must hold", so A's fields are a precondition
      // that must be known before B's fields can be meaningfully checked.
      // Without this edge, e.g. `this.total_ram > 32 -> this.tier ==
      // "enterprise"` could schedule `tier` before `total_ram` is ever
      // assigned, and the implication would be checked against an
      // unassigned value -- silently never enforced.
      const auto& binExpr =
          std::get<binder::BoundBinaryExpr>(trigger.constraint->expr->value);
      auto antecedentDeps = extractDeps(*binExpr.left, trigger.instancePrefix);
      auto consequentDeps = extractDeps(*binExpr.right, trigger.instancePrefix);
      for (const auto& a : antecedentDeps) {
        for (const auto& c : consequentDeps) {
          if (a != c) {
            adj[a].push_back(c);
            inDegree[c]++;
          }
        }
      }
    }
  }

  // Execute Kahn's Topological Sort
  ExecutionSchedule schedule;
  std::queue<std::string> q;

  for (const auto& node : allNodes) {
    if (inDegree[node] == 0) q.push(node);
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
  // Attach Triggers to the latest generated dependency
  for (const auto& trigger : allTriggers) {
    if (trigger.constraint->isDeterministicPossible) {
      // Apply the fallback so the trigger is saved to "total", not ""
      std::string target =
          trigger.constraint->target.empty()
              ? trigger.ownerFieldPath
              : trigger.instancePrefix + trigger.constraint->target;
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