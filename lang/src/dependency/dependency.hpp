#pragma once

#include "expression.hpp"
#include "graph.hpp"
#include "statements.hpp"
#include "types.hpp"

namespace invariants::dependency {
class DependencyGraphGen {
 private:
  Graph graph;
  std::optional<GraphNodeId> currSrc;

  template <typename T>
  void visit(const T& node) {
    std::visit(*this, node);
  }

 public:
  DependencyGraphGen() = default;

  Graph build() &&;
  const Graph& getGraph() const;

  // Expressions
  void operator()(const ast::LiteralExpr& e);
  void operator()(const ast::IdentifierExpr& e);
  void operator()(const ast::ThisExpr&);
  void operator()(const ast::ListExpr& e);
  void operator()(const ast::GroupingExpr& e);
  void operator()(const ast::PostfixExpr& e);
  void operator()(const ast::MemberAccessOp& e);
  void operator()(const ast::IndexOp& e);
  void operator()(const ast::UnaryExpr& e);
  void operator()(const ast::BinaryExpr& e);

  // Statements
  void operator()(const ast::ConstraintStmt& e);
  void operator()(const ast::FieldStmt& e);
  void operator()(const ast::InvariantStmt&);
  void operator()(const ast::SpecStmt& e);
  void operator()(const ast::ModuleStmt& e);

  // Types
  void operator()(const ast::BuiltinType& e);
  void operator()(const ast::SimpleType& e);
  void operator()(const ast::ArrayType&);
  void operator()(const ast::MapType& e);
};

}  // namespace invariants::dependency