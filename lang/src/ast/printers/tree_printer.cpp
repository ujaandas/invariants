#include "tree_printer.hpp"

#include <sstream>

#include "plain_printer.hpp"

using namespace invariants::ast::printers;

namespace {

std::string literal_to_string(
    const std::variant<double, std::string, bool, std::nullptr_t>& value) {
  return std::visit(
      [](const auto& v) -> std::string {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::string>) {
          return "\"" + v + "\"";
        } else if constexpr (std::is_same_v<T, bool>) {
          return v ? "true" : "false";
        } else if constexpr (std::is_same_v<T, std::nullptr_t>) {
          return "null";
        } else {
          return std::to_string(v);
        }
      },
      value);
}

TreeNode make_node(std::string label, std::vector<TreeNode> children = {}) {
  return TreeNode{std::move(label), std::move(children)};
}

}  // namespace

TreeNode Tree::attach(const std::string& edge, TreeNode node) {
  if (edge.empty()) return node;
  node.label = edge + ": " + node.label;
  return node;
}

std::string Tree::render(const TreeNode& root) const {
  std::vector<std::string> lines;
  render(root, "", true, lines, true);

  std::ostringstream out;
  for (size_t i = 0; i < lines.size(); ++i) {
    if (i > 0) out << '\n';
    out << lines[i];
  }
  return out.str();
}

void Tree::render(const TreeNode& node, const std::string& prefix, bool isLast,
                  std::vector<std::string>& lines, bool isRoot) const {
  if (isRoot) {
    lines.push_back(prefix + node.label);
  } else {
    lines.push_back(prefix + (isLast ? "\\-- " : "/-- ") + node.label);
  }

  const std::string childPrefix =
      isRoot ? std::string{} : prefix + (isLast ? "    " : "|   ");

  for (size_t i = 0; i < node.children.size(); ++i) {
    render(node.children[i], childPrefix, i + 1 == node.children.size(), lines,
           false);
  }
}

TreeNode Tree::operator()(const LiteralExpr& e) const {
  return make_node("Literal " + literal_to_string(e.value));
}

TreeNode Tree::operator()(const IdentifierExpr& e) const {
  return make_node("Identifier " + e.name);
}

TreeNode Tree::operator()(const ThisExpr&) const { return make_node("This"); }

TreeNode Tree::operator()(const ListExpr& e) const {
  std::vector<TreeNode> children;
  children.reserve(e.elements.size());
  for (size_t i = 0; i < e.elements.size(); ++i) {
    children.emplace_back(
        attach("element[" + std::to_string(i) + "]", build(*e.elements[i])));
  }
  return make_node("List", std::move(children));
}

TreeNode Tree::operator()(const GroupingExpr& e) const {
  return make_node("Grouping", {attach("expression", build(*e.expression))});
}

TreeNode Tree::operator()(const PostfixExpr& e) const {
  std::vector<TreeNode> children;
  children.emplace_back(attach("base", build(*e.base)));
  for (size_t i = 0; i < e.ops.size(); ++i) {
    children.emplace_back(
        attach("op[" + std::to_string(i) + "]", build(e.ops[i])));
  }
  return make_node("Postfix", std::move(children));
}

TreeNode Tree::operator()(const MemberAccessOp& e) const {
  return make_node("." + e.member);
}

TreeNode Tree::operator()(const IndexOp& e) const {
  return make_node("Index", {attach("index", build(*e.index))});
}

TreeNode Tree::operator()(const UnaryExpr& e) const {
  return make_node("Unary " + to_string(e.op),
                   {attach("operand", build(*e.operand))});
}

TreeNode Tree::operator()(const BinaryExpr& e) const {
  std::vector<TreeNode> children;
  children.emplace_back(attach("left", build(*e.left)));
  children.emplace_back(attach("right", build(*e.right)));
  return make_node("Binary " + to_string(e.op), std::move(children));
}

TreeNode Tree::operator()(const ConstraintStmt& e) const {
  return make_node("Constraint", {attach("expression", build(*e.expression))});
}

TreeNode Tree::operator()(const FieldStmt& e) const {
  std::vector<TreeNode> children;
  children.emplace_back(attach("type", build(*e.type)));
  for (size_t i = 0; i < e.constraints.size(); ++i) {
    children.emplace_back(attach("constraint[" + std::to_string(i) + "]",
                                 build(*e.constraints[i])));
  }
  return make_node("Field " + e.identifier, std::move(children));
}

TreeNode Tree::operator()(const InvariantStmt& e) const {
  std::vector<TreeNode> children;
  for (size_t i = 0; i < e.constraints.size(); ++i) {
    children.emplace_back(attach("constraint[" + std::to_string(i) + "]",
                                 build(*e.constraints[i])));
  }
  return make_node("Invariant " + e.identifier, std::move(children));
}

TreeNode Tree::operator()(const SpecStmt& e) const {
  std::vector<TreeNode> children;
  children.reserve(e.members.size());
  for (size_t i = 0; i < e.members.size(); ++i) {
    children.emplace_back(
        attach("member[" + std::to_string(i) + "]", build(e.members[i])));
  }
  return make_node("Spec " + e.identifier, std::move(children));
}

TreeNode Tree::operator()(const ModuleStmt& e) const {
  std::vector<TreeNode> children;
  children.reserve(e.specs.size());
  for (size_t i = 0; i < e.specs.size(); ++i) {
    children.emplace_back(
        attach("spec[" + std::to_string(i) + "]", build(*e.specs[i])));
  }
  return make_node("Module", std::move(children));
}

TreeNode Tree::operator()(const BuiltinType& e) const {
  switch (e) {
    case BuiltinType::Number:
      return make_node("Number");
    case BuiltinType::Integer:
      return make_node("Integer");
    case BuiltinType::String:
      return make_node("String");
    case BuiltinType::Boolean:
      return make_node("Boolean");
  }
  return make_node("?");
}

TreeNode Tree::operator()(const SimpleType& e) const {
  return std::visit(
      [this](const auto& v) -> TreeNode {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, BuiltinType>) {
          return (*this)(v);
        } else {
          return make_node(v);
        }
      },
      e.value);
}

TreeNode Tree::operator()(const ArrayType& e) const {
  return make_node("Array", {attach("element", build(e.element->value))});
}

TreeNode Tree::operator()(const MapType& e) const {
  return make_node("Map", {attach("key", build(e.key->value)),
                           attach("value", build(e.value->value))});
}