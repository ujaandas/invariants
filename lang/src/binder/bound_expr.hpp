#pragma once

#include <cstddef>
#include <memory>
#include <variant>

#include "expression.hpp"
#include "symbol_table.hpp"
namespace invariants::binder {

struct BoundExpr;
using BoundExprPtr = std::unique_ptr<BoundExpr>;

// Literal
struct BoundLiteralExpr {
  std::variant<double, int, std::string, bool, std::nullptr_t> value;
};

// Field access
struct BoundFieldAccessExpr {
  const FieldSymbol* field;
  std::string flattenedPath;  // Flattens nested structures for runtime
};

// Value keyword
struct BoundValueAccessExpr {
  ResolvedType expectedType;
};

// Unary op
struct BoundUnaryExpr {
  ast::UnaryOp op;
  BoundExprPtr operand;
};

// Binary op
struct BoundBinaryExpr {
  BoundExprPtr left;
  ast::BinaryOp op;
  BoundExprPtr right;
};

// List
struct BoundListExpr {
  std::vector<BoundExprPtr> elements;
};

struct BoundExpr {
  using ExprT =
      std::variant<BoundLiteralExpr, BoundFieldAccessExpr, BoundValueAccessExpr,
                   BoundUnaryExpr, BoundBinaryExpr, BoundListExpr>;
  ExprT value;
  ResolvedType type;

  template <typename T>
    requires(!std::same_as<std::decay_t<T>, BoundExpr>)
  BoundExpr(T&& v, ResolvedType t)
      : value(std::forward<T>(v)), type(std::move(t)) {}
};

}  // namespace invariants::binder