#pragma once

#include <cstddef>

#include "symbol_table.hpp"

namespace invariants::analysis {

class DependencyGraph {
 public:
  explicit DependencyGraph(std::size_t numFields);

  // Add dependency where from is gen before to
  void addEdge(binder::FieldId, binder::FieldId);
};

}  // namespace invariants::analysis