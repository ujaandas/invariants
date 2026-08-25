#pragma once

#include "bound_expr.hpp"
#include "bound_stmt.hpp"
#include "expression.hpp"
#include "statements.hpp"
#include "symbol_table.hpp"

namespace invariants::binder {

class Binder {
 public:
  BoundModule bind(const ast::ModuleStmt& moduleAst);

  const SymbolTable& getSt() const { return table; }

 private:
  SymbolTable table;

  // Context trackers
  SpecSymbol* activeSpec = nullptr;
  FieldSymbol* activeField = nullptr;  // Set only inside field constraints

  BoundSpec bindSpec(const ast::SpecStmt& specAst);
  BoundField bindField(const ast::FieldStmt& fieldAst);
  BoundConstraint bindConstraint(const ast::ConstraintStmt& constraintAst);
  BoundInvariant bindInvariant(const ast::InvariantStmt& invariantAst);

  ResolvedType bindType(const ast::Type& typeAst);

  BoundExprPtr bindExpr(const ast::Expr& exprAst);

  // Expression helpers
  BoundExprPtr bindLiteral(const ast::LiteralExpr& expr);
  BoundExprPtr bindIdentifier(const ast::IdentifierExpr& expr);
  BoundExprPtr bindPostfix(const ast::PostfixExpr& expr);
  BoundExprPtr bindUnary(const ast::UnaryExpr& expr);
  BoundExprPtr bindBinary(const ast::BinaryExpr& expr);
};

}  // namespace invariants::binder