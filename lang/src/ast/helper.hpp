#pragma once

#include <memory>
#include <vector>
namespace invariants::ast {

template <typename T>
bool ptr_equal(const std::unique_ptr<T>& a, const std::unique_ptr<T>& b) {
  if (!a || !b) return !a && !b;
  return *a == *b;
}

template <typename T>
bool ptr_vector_equal(const std::vector<std::unique_ptr<T>>& a,
                      const std::vector<std::unique_ptr<T>>& b) {
  if (a.size() != b.size()) {
    return false;
  }

  for (size_t i = 0; i < a.size(); ++i) {
    if (!ptr_equal(a[i], b[i])) {
      return false;
    }
  }

  return true;
}

}  // namespace invariants::ast