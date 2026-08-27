#pragma once

#include <cstddef>
#include <vector>

#include "symbol_table.hpp"

namespace invariants::analysis {

class DependencyGraph {
 public:
  explicit DependencyGraph(std::size_t numFields);

  // Add dependency where from is gen before to
  void addEdge(binder::FieldId, binder::FieldId);

  bool hasEdge(binder::FieldId, binder::FieldId) const;
  std::size_t size() const;

  // Returns all dependencies of field
  const std::vector<binder::FieldId>& getDependencies(binder::FieldId) const;

  // Returns all dependents (fields that depend on) of field
  const std::vector<binder::FieldId>& getDependents(binder::FieldId) const;

  // Compute gen order
  std::vector<binder::FieldId> order() const;

 private:
  std::size_t numFields;
  std::vector<std::vector<binder::FieldId>> outgoingAdj;
  std::vector<std::vector<binder::FieldId>> incomingAdj;
  std::vector<std::size_t> inDeg;
};

}  // namespace invariants::analysis