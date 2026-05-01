#pragma once

#include <cstdint>
#include <memory>
#include <variant>
#include <vector>

namespace invariants::ast {

struct Node {
  virtual ~Node() = default;
};

struct Expr : Node {
  virtual ~Expr() = default;
};

using ExprPtr = std::unique_ptr<Expr>;

struct LiteralExpr : Expr {
  std::variant<double, std::string, bool, std::nullptr_t> value;
};

struct IdentifierExpr : Expr {
  std::string name;
};

using IdentifierPtr = std::unique_ptr<IdentifierExpr>;

struct ThisExpr : Expr {};

struct ListExpr : Expr {
  std::vector<ExprPtr> elements;
};

struct GroupingExpr : Expr {
  ExprPtr expression;
};

struct PostfixOp {
  virtual ~PostfixOp() = default;
};

using PostfixOpPtr = std::unique_ptr<PostfixOp>;

struct MemberAccessOp : PostfixOp {
  std::string member;
};

struct IndexOp : PostfixOp {
  ExprPtr index;
};

struct PostfixExpr : Expr {
  ExprPtr base;
  std::vector<PostfixOpPtr> ops;
};

enum class UnaryOp : std::uint8_t { Not, Negate };

struct UnaryExpr : Expr {
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

struct BinaryExpr : Expr {
  ExprPtr left;
  BinaryOp op;
  ExprPtr right;
};
}  // namespace invariants::ast