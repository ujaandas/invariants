#include "dependency.hpp"

#include <functional>
#include <variant>

using namespace invariants::dependency;

std::string DependencyGraphGen::qualifyName(std::string_view name) const {
  if (currSpecName.empty()) {
    return std::string(name);
  }
  return currSpecName + "." + std::string(name);
}

void DependencyGraphGen::operator()(const ast::ModuleStmt& e) {
  for (const auto& spec : e.specs) {
    if (!spec) continue;
    (*this)(*spec);
  }
}

void DependencyGraphGen::operator()(const ast::SpecStmt& e) {
  std::string oldSpec = currSpecName;
  currSpecName = e.identifier;
  nextInvariantId = 0;

  for (const auto& member : e.members) {
    std::visit(std::ref(*this), member);
  }

  currSpecName = oldSpec;
}

void DependencyGraphGen::operator()(const ast::FieldStmt& e) {
  std::string oldField = currFieldName;
  currFieldName = e.identifier;

  // Register this field as a node in the graph
  graph.addNode(qualifyName(currFieldName));

  if (e.type) {
    std::visit(std::ref(*this), e.type->value);
  }

  for (const auto& constraint : e.constraints) {
    if (constraint) {
      (*this)(*constraint);
    }
  }

  currFieldName = oldField;
}

void DependencyGraphGen::operator()(const ast::InvariantStmt& e) {
  std::string oldField = currFieldName;
  // Synthetic node identifier for spec-level invariants
  currFieldName = "inv@" + std::to_string(nextInvariantId++);

  graph.addNode(qualifyName(currFieldName));

  for (const auto& constraint : e.constraints) {
    if (constraint) {
      (*this)(*constraint);
    }
  }

  currFieldName = oldField;
}

void DependencyGraphGen::operator()(const ast::ConstraintStmt& e) {
  if (e.expression) {
    std::visit(std::ref(*this), e.expression->value);
  }
}

void DependencyGraphGen::operator()(const ast::PostfixExpr& e) {
  if (!e.base) return;

  // Unwrap parentheses to inspect the true base expression
  const ast::Expr* baseExpr = e.base.get();
  while (baseExpr &&
         std::holds_alternative<ast::GroupingExpr>(baseExpr->value)) {
    const auto& grouping = std::get<ast::GroupingExpr>(baseExpr->value);
    baseExpr = grouping.expression.get();
  }

  bool isBaseThis =
      baseExpr && std::holds_alternative<ast::ThisExpr>(baseExpr->value);

  if (isBaseThis) {
    bool isFirstMemberOp = true;

    for (const auto& op : e.ops) {
      if (std::holds_alternative<ast::MemberAccessOp>(op)) {
        const auto& memOp = std::get<ast::MemberAccessOp>(op);

        // The first member accessed on 'this' (e.g., this.fieldName) is the
        // dependency
        if (isFirstMemberOp && !currFieldName.empty()) {
          graph.addEdge(qualifyName(memOp.member), qualifyName(currFieldName));
          isFirstMemberOp = false;
        }
      } else {
        // Index expressions like this.items[this.index] contain expressions to
        // visit
        const auto& idxOp = std::get<ast::IndexOp>(op);
        if (idxOp.index) {
          std::visit(std::ref(*this), idxOp.index->value);
        }
        isFirstMemberOp = false;
      }
    }
  } else {
    // If base is not 'this', visit it normally and traverse index expressions
    std::visit(std::ref(*this), e.base->value);

    for (const auto& op : e.ops) {
      if (std::holds_alternative<ast::IndexOp>(op)) {
        const auto& idxOp = std::get<ast::IndexOp>(op);
        if (idxOp.index) {
          std::visit(std::ref(*this), idxOp.index->value);
        }
      }
    }
  }
}

void DependencyGraphGen::operator()(const ast::GroupingExpr& e) {
  if (e.expression) {
    std::visit(std::ref(*this), e.expression->value);
  }
}

void DependencyGraphGen::operator()(const ast::ListExpr& e) {
  for (const auto& elem : e.elements) {
    if (elem) {
      std::visit(std::ref(*this), elem->value);
    }
  }
}

void DependencyGraphGen::operator()(const ast::UnaryExpr& e) {
  if (e.operand) {
    std::visit(std::ref(*this), e.operand->value);
  }
}

void DependencyGraphGen::operator()(const ast::BinaryExpr& e) {
  if (e.left) std::visit(std::ref(*this), e.left->value);
  if (e.right) std::visit(std::ref(*this), e.right->value);
}

void DependencyGraphGen::operator()(const ast::IdentifierExpr&) {
  // 'value' refers to the current field itself; no external dependency is
  // added.
}

void DependencyGraphGen::operator()(const ast::ThisExpr&) {}
void DependencyGraphGen::operator()(const ast::LiteralExpr&) {}

void DependencyGraphGen::operator()(const ast::MemberAccessOp&) {}

void DependencyGraphGen::operator()(const ast::IndexOp& e) {
  if (e.index) {
    std::visit(std::ref(*this), e.index->value);
  }
}

void DependencyGraphGen::operator()(const ast::BuiltinType&) {}

void DependencyGraphGen::operator()(const ast::SimpleType&) {}

void DependencyGraphGen::operator()(const ast::ArrayType& e) {
  if (e.element) {
    std::visit(std::ref(*this), e.element->value);
  }
}

void DependencyGraphGen::operator()(const ast::MapType& e) {
  if (e.key) {
    std::visit(std::ref(*this), e.key->value);
  }
  if (e.value) {
    std::visit(std::ref(*this), e.value->value);
  }
}