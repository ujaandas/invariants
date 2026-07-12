#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "ptr_eq_helper.hpp"

namespace invariants::ast {

// Forward decl
struct Expr;
using ExprPtr = std::unique_ptr<Expr>;

struct LiteralExpr {
  std::variant<double, std::string, bool, std::nullptr_t> value;
  bool operator==(const LiteralExpr&) const = default;
};

struct IdentifierExpr {
  std::string name;
  bool operator==(const IdentifierExpr&) const = default;
};

struct ThisExpr {
  bool operator==(const ThisExpr&) const = default;
};

struct ListExpr {
  std::vector<ExprPtr> elements;
};

inline bool operator==(const ListExpr& a, const ListExpr& b) {
  return ptr_vector_equal(a.elements, b.elements);
}

struct GroupingExpr {
  ExprPtr expression;
};

inline bool operator==(const GroupingExpr& a, const GroupingExpr& b) {
  return ptr_equal(a.expression, b.expression);
}

struct MemberAccessOp {
  std::string member;
  bool operator==(const MemberAccessOp&) const = default;
};

struct IndexOp {
  ExprPtr index;
};

inline bool operator==(const IndexOp& a, const IndexOp& b) {
  return ptr_equal(a.index, b.index);
}

using PostfixOp = std::variant<MemberAccessOp, IndexOp>;

struct PostfixExpr {
  ExprPtr base;
  std::vector<PostfixOp> ops;
};

inline bool operator==(const PostfixExpr& a, const PostfixExpr& b) {
  return ptr_equal(a.base, b.base) && a.ops == b.ops;
}

enum class UnaryOp : std::uint8_t { Not, Negate };

struct UnaryExpr {
  UnaryOp op;
  ExprPtr operand;
};

inline bool operator==(const UnaryExpr& a, const UnaryExpr& b) {
  return a.op == b.op && ptr_equal(a.operand, b.operand);
}

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

inline bool operator==(const BinaryExpr& a, const BinaryExpr& b) {
  return ptr_equal(a.left, b.left) && a.op == b.op &&
         ptr_equal(a.right, b.right);
}

struct Expr {
  using ExprT = std::variant<LiteralExpr, IdentifierExpr, ThisExpr, ListExpr,
                             GroupingExpr, PostfixExpr, UnaryExpr, BinaryExpr>;

  ExprT value;

  template <typename T>
    requires(!std::same_as<std::decay_t<T>, Expr>)
  explicit Expr(T&& v) : value(std::forward<T>(v)) {}

  bool operator==(const Expr&) const = default;

  friend std::ostream& operator<<(std::ostream& os, const Expr& expr);
};

}  // namespace invariants::ast