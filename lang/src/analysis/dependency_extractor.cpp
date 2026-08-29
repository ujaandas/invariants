#include "dependency_extractor.hpp"

namespace invariants::analysis {

void DependencyExtractor::operator()(const binder::BoundLiteralExpr&) const {
  // Doesn't create dependencies
}

void DependencyExtractor::operator()(
    const binder::BoundValueAccessExpr&) const {
  // Doesn't create dependencies
}

void DependencyExtractor::operator()(
    const binder::BoundFieldAccessExpr& e) const {
  if (e.field) {
    deps.insert(e.field->id);
  }
}

void DependencyExtractor::operator()(const binder::BoundUnaryExpr& e) const {
  if (e.operand) {
    std::visit(*this, e.operand->value);
  }
}

void DependencyExtractor::operator()(const binder::BoundBinaryExpr& e) const {
  if (e.left) {
    std::visit(*this, e.left->value);
  }
  if (e.right) {
    std::visit(*this, e.right->value);
  }
}

}  // namespace invariants::analysis