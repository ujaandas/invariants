#pragma once

#include <cstddef>
#include <vector>

#include "symbol_table.hpp"

namespace invariants::analysis {

using FieldId = binder::FieldId;

class DependencyGraph {
 public:
  explicit DependencyGraph(std::size_t numFields);

  // Add dependency where from is gen before to
  void addEdge(FieldId, FieldId);

  bool hasEdge(FieldId, FieldId) const;
  std::size_t size() const;

  // Returns all dependencies of field
  const std::vector<FieldId>& getDependencies(FieldId) const;

  // Returns all dependents (fields that depend on) of field
  const std::vector<FieldId>& getDependents(FieldId) const;

  // Compute gen order
  std::vector<FieldId> order() const;

 private:
  std::size_t numFields;
  std::vector<std::vector<FieldId>> outgoingAdj;
  std::vector<std::vector<FieldId>> incomingAdj;
  std::vector<std::size_t> inDeg;
};

}  // namespace invariants::analysis