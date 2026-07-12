#pragma once

#include <string>
#include <vector>

#include "../ast_expr.hpp"
#include "../ast_stmt.hpp"
#include "../ast_types.hpp"

namespace invariants::ast::printers {

struct TreeNode {
  std::string label;
  std::vector<TreeNode> children;
};

struct Tree {
  TreeNode operator()(const LiteralExpr& e) const;
  TreeNode operator()(const IdentifierExpr& e) const;
  TreeNode operator()(const ThisExpr&) const;
  TreeNode operator()(const ListExpr& e) const;
  TreeNode operator()(const GroupingExpr& e) const;
  TreeNode operator()(const PostfixExpr& e) const;
  TreeNode operator()(const MemberAccessOp& e) const;
  TreeNode operator()(const IndexOp& e) const;
  TreeNode operator()(const UnaryExpr& e) const;
  TreeNode operator()(const BinaryExpr& e) const;

  TreeNode operator()(const ConstraintStmt& e) const;
  TreeNode operator()(const FieldStmt& e) const;
  TreeNode operator()(const InvariantStmt& e) const;
  TreeNode operator()(const SpecStmt& e) const;
  TreeNode operator()(const ModuleStmt& e) const;

  TreeNode operator()(const BuiltinType& e) const;
  TreeNode operator()(const SimpleType& e) const;
  TreeNode operator()(const ArrayType& e) const;
  TreeNode operator()(const MapType& e) const;

  template <typename T>
  std::string print(const T& v) const {
    return render(build(v));
  }

 private:
  template <typename T>
  TreeNode build(const T& v) const {
    if constexpr (requires { v.value; }) {
      return std::visit(*this, v.value);  // wrapper
    } else if constexpr (requires { std::visit(*this, v); }) {
      return std::visit(*this, v);  // variant
    } else {
      return (*this)(v);  // plain type
    }
  }

  static TreeNode attach(const std::string& edge, TreeNode node);
  std::string render(const TreeNode& root) const;
  void render(const TreeNode& node, const std::string& prefix, bool isLast,
              std::vector<std::string>& lines, bool isRoot) const;
};

}  // namespace invariants::ast::printers