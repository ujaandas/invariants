#pragma once

#include <string>
#include <variant>

#include "../expression.hpp"
#include "../statements.hpp"
#include "../types.hpp"

namespace invariants::ast::visitors {

struct Printer {
  // Expressions
  std::string operator()(const LiteralExpr& e) const;
  std::string operator()(const IdentifierExpr& e) const;
  std::string operator()(const ThisExpr&) const;
  std::string operator()(const ListExpr& e) const;
  std::string operator()(const GroupingExpr& e) const;
  std::string operator()(const PostfixExpr& e) const;
  std::string operator()(const MemberAccessOp& e) const;
  std::string operator()(const IndexOp& e) const;
  std::string operator()(const UnaryExpr& e) const;
  std::string operator()(const BinaryExpr& e) const;

  // Statements
  std::string operator()(const ConstraintStmt& e) const;
  std::string operator()(const FieldStmt& e) const;
  std::string operator()(const InvariantStmt&) const;
  std::string operator()(const SpecStmt& e) const;
  std::string operator()(const ModuleStmt& e) const;

  // Types
  std::string operator()(const BuiltinType& e) const;
  std::string operator()(const SimpleType& e) const;
  std::string operator()(const ArrayType&) const;
  std::string operator()(const MapType& e) const;

  template <typename T>
  std::string print(const T& v) const {
    if constexpr (requires { v.value; }) {
      return std::visit(*this, v.value);  // wrapper
    } else if constexpr (requires { std::visit(*this, v); }) {
      return std::visit(*this, v);  // variant
    } else {
      return (*this)(v);  // plain type
    }
  }
};

inline std::string to_string(UnaryOp op) {
  switch (op) {
    case UnaryOp::Not:
      return "!";
    case UnaryOp::Negate:
      return "-";
  }
  return "?";
}

inline std::string to_string(BinaryOp op) {
  switch (op) {
    case BinaryOp::Add:
      return "+";
    case BinaryOp::Subtract:
      return "-";
    case BinaryOp::Multiply:
      return "*";
    case BinaryOp::Divide:
      return "/";
    case BinaryOp::Modulo:
      return "%";
    case BinaryOp::And:
      return "and";
    case BinaryOp::Or:
      return "or";
    case BinaryOp::Equal:
      return "==";
    case BinaryOp::NotEqual:
      return "!=";
    case BinaryOp::Greater:
      return ">";
    case BinaryOp::GreaterEqual:
      return ">=";
    case BinaryOp::Less:
      return "<";
    case BinaryOp::LessEqual:
      return "<=";
    case BinaryOp::In:
      return "in";
    case BinaryOp::NotIn:
      return "not in";
    case BinaryOp::Imply:
      return "=>";
  }
  return "?";
}

}  // namespace invariants::ast::visitors
