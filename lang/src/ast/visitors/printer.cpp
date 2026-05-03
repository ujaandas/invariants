#include "printer.hpp"

#include <sstream>

using namespace invariants::ast::visitors;

std::string Printer::operator()(const LiteralExpr& e) const {
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
      e.value);
}

std::string Printer::operator()(const IdentifierExpr& e) const {
  return e.name;
}

std::string Printer::operator()(const ThisExpr&) const { return "this"; }

std::string Printer::operator()(const ListExpr& e) const {
  std::ostringstream out;
  out << "[";
  for (size_t i = 0; i < e.elements.size(); ++i) {
    if (i > 0) out << ", ";
    out << print(*e.elements[i]);
  }
  out << "]";
  return out.str();
}

std::string Printer::operator()(const GroupingExpr& e) const {
  return "(" + print(*e.expression) + ")";
}

std::string Printer::operator()(const PostfixExpr& e) const {
  std::string s = print(*e.base);
  for (const auto& op : e.ops) {
    s += print(op);
  }
  return s;
}

std::string Printer::operator()(const MemberAccessOp& e) const {
  return "." + e.member;
}

std::string Printer::operator()(const IndexOp& e) const {
  return "[" + print(*e.index) + "]";
}

std::string Printer::operator()(const UnaryExpr& e) const {
  return "(" + to_string(e.op) + print(*e.operand) + ")";
}

std::string Printer::operator()(const BinaryExpr& e) const {
  return "(" + print(*e.left) + " " + to_string(e.op) + " " + print(*e.right) +
         ")";
}

std::string Printer::operator()(const ConstraintStmt& e) const {
  return print(*e.expression) + ";";
}

std::string Printer::operator()(const FieldStmt& e) const {
  std::ostringstream out;
  out << "field " << e.identifier << ": " << print(*e.type) << " {";
  if (!e.constraints.empty()) {
    out << " ";
    for (size_t i = 0; i < e.constraints.size(); ++i) {
      if (i > 0) out << " ";
      out << print(*e.constraints[i]);
    }
    out << " ";
  }
  out << "}";
  return out.str();
}

std::string Printer::operator()(const InvariantStmt& e) const {
  std::ostringstream out;
  out << "invariant " << e.identifier << " {";
  if (!e.constraints.empty()) {
    out << " ";
    for (size_t i = 0; i < e.constraints.size(); ++i) {
      if (i > 0) out << " ";
      out << print(*e.constraints[i]);
    }
    out << " ";
  }
  out << "}";
  return out.str();
}

std::string Printer::operator()(const SpecStmt& e) const {
  std::ostringstream out;
  out << "spec " << e.identifier << " {";
  if (!e.members.empty()) {
    out << " ";
    for (size_t i = 0; i < e.members.size(); ++i) {
      if (i > 0) out << " ";
      out << print(e.members[i]);
    }
    out << " ";
  }
  out << "}";
  return out.str();
}

std::string Printer::operator()(const ModuleStmt& e) const {
  std::ostringstream out;
  for (size_t i = 0; i < e.specs.size(); ++i) {
    if (i > 0) out << " ";
    out << print(*e.specs[i]);
  }
  return out.str();
}

std::string Printer::operator()(const BuiltinType& e) const {
  switch (e) {
    case BuiltinType::Number:
      return "Number";
    case BuiltinType::Integer:
      return "Integer";
    case BuiltinType::String:
      return "String";
    case BuiltinType::Boolean:
      return "Boolean";
  }
  return "?";
}

std::string Printer::operator()(const SimpleType& e) const {
  return std::visit(
      [this](const auto& v) -> std::string {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, BuiltinType>) {
          return (*this)(v);
        } else {
          return v;
        }
      },
      e.value);
}

std::string Printer::operator()(const ArrayType& e) const {
  return "Array<" + print(e.element->value) + ">";
}

std::string Printer::operator()(const MapType& e) const {
  return "Map<" + print(e.key->value) + ", " + print(e.value->value) + ">";
}