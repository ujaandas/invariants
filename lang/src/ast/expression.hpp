#pragma once

#include <cstdint>
#include <memory>
#include <variant>
#include <vector>

namespace invariants::ast {

// Forward decl
struct Expr;
using ExprPtr = std::unique_ptr<Expr>;

struct LiteralExpr {
  std::variant<double, std::string, bool, std::nullptr_t> value;
};

struct IdentifierExpr {
  std::string name;
};

struct ThisExpr {};

struct ListExpr {
  std::vector<ExprPtr> elements;
};

struct GroupingExpr {
  ExprPtr expression;
};

struct MemberAccessOp {
  std::string member;
};

struct IndexOp {
  ExprPtr index;
};

using PostfixOp = std::variant<MemberAccessOp, IndexOp>;

struct PostfixExpr {
  ExprPtr base;
  std::vector<PostfixOp> ops;
};

enum class UnaryOp : std::uint8_t { Not, Negate };

struct UnaryExpr {
  UnaryOp op;
  ExprPtr operand;
};

enum class BinaryOp : std::uint8_t {
  Add,
  Subtract,
  Multiply,
  Divide,
  Modulo,
  And,
  Or,
  Equal,
  NotEqual,
  Greater,
  GreaterEqual,
  Less,
  LessEqual,
  In,
  NotIn,
  Imply
};

struct BinaryExpr {
  ExprPtr left;
  BinaryOp op;
  ExprPtr right;
};

struct Expr {
  using ExprT = std::variant<LiteralExpr, IdentifierExpr, ThisExpr, ListExpr,
                             GroupingExpr, PostfixExpr, UnaryExpr, BinaryExpr>;

  ExprT value;

  template <typename T>
  Expr(T v) : value(std::move(v)) {}
};

}  // namespace invariants::ast