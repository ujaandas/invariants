#pragma once

#include <cstddef>
#include <string>
#include <string_view>

#include "expression.hpp"
#include "graph.hpp"
#include "statements.hpp"
#include "types.hpp"

namespace invariants::dependency {

class DependencyGraphGen {
 private:
  Graph graph;
  std::string currSpecName;
  std::string currFieldName;
  std::size_t nextInvariantId = 0;

  std::string qualifyName(std::string_view name) const;

 public:
  DependencyGraphGen() = default;

  // Access the generated graph
  const Graph& getGraph() const { return graph; }
  Graph takeGraph() && { return std::move(graph); }

  // Expressions
  void operator()(const ast::LiteralExpr& e);
  void operator()(const ast::IdentifierExpr& e);
  void operator()(const ast::ThisExpr& e);
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
  void operator()(const ast::InvariantStmt& e);
  void operator()(const ast::SpecStmt& e);
  void operator()(const ast::ModuleStmt& e);

  // Types
  void operator()(const ast::BuiltinType& e);
  void operator()(const ast::SimpleType& e);
  void operator()(const ast::ArrayType& e);
  void operator()(const ast::MapType& e);
};

}  // namespace invariants::dependency