#include "resolver.hpp"

#include <memory>
#include <stdexcept>

#include "statements.hpp"
#include "symbol_table.hpp"

using namespace invariants::resolver;

void Resolver::operator()(const ast::ModuleStmt& e) {
  for (const auto& spec : e.specs) {
    if (!spec) continue;
    (*this)(*spec);
  }
}

void Resolver::operator()(const ast::SpecStmt& e) {
  auto spec =
      std::make_unique<SpecSymbol>(SpecSymbol{nextSpecId++, e.identifier, &e});

  if (!table.add_spec(e.identifier, std::move(spec))) {
    throw std::runtime_error("Duplicate specification.");
  }

  std::string oldCtx = currSpecName;
  currSpecName = e.identifier;
  nextFieldId = 0;

  for (const auto& field : e.members) {
    std::visit(std::ref(*this), field);
  }

  currSpecName = oldCtx;
}

void Resolver::operator()(const ast::FieldStmt& e) {
  if (!e.type) {
    throw std::runtime_error("Field '" + e.identifier +
                             "' is missing a type definition.");
  }

  std::visit(std::ref(*this), e.type->value);

  auto field = std::make_unique<FieldSymbol>(FieldSymbol{.id = nextFieldId++,
                                                         .name = e.identifier,
                                                         .type = e.type.get(),
                                                         .decl = &e});

  if (!table.add_field(currSpecName, e.identifier, std::move(field))) {
    throw std::runtime_error("Duplicate field definition: " + e.identifier +
                             " in spec " + currSpecName);
  }

  std::string oldField = currFieldName;
  currFieldName = e.identifier;

  for (const auto& constraint : e.constraints) {
    if (constraint) {
      (*this)(*constraint);
    }
  }

  currFieldName = oldField;
}

void Resolver::operator()(const ast::SimpleType& e) {
  // SimpleType is either a string, which repr a spec, or a builtin, which is
  // fine anyways
  if (std::holds_alternative<std::string>(e.value)) {
    const auto& customType = std::get<std::string>(e.value);
    if (!table.lookup_spec(customType)) {
      throw std::runtime_error("Unknown type identifier: " + customType);
    }
  }
}

void Resolver::operator()(const ast::BuiltinType& e) {
  // Always valid
}

void Resolver::operator()(const ast::ArrayType& e) {
  if (e.element) {
    std::visit(std::ref(*this), e.element->value);
  }
}

void Resolver::operator()(const ast::MapType& e) {
  if (e.key) {
    std::visit(std::ref(*this), e.key->value);
  }
  if (e.value) {
    std::visit(std::ref(*this), e.value->value);
  }
}

void Resolver::operator()(const ast::InvariantStmt& e) {
  for (const auto& constraint : e.constraints) {
    if (constraint) {
      (*this)(*constraint);
    }
  }
}

void Resolver::operator()(const ast::ConstraintStmt& e) {
  if (e.expression) {
    std::visit(std::ref(*this), e.expression->value);
  }
}

void Resolver::operator()(const ast::LiteralExpr& e) {
  // Don't need mapping
}

void Resolver::operator()(const ast::IdentifierExpr& e) {
  // Only allowed if explicit 'value'
  if (e.name == "value") {
    if (currFieldName.empty()) {
      throw std::runtime_error(
          "The 'value' keyword can only be used inside field constraints.");
    }
  } else {
    throw std::runtime_error("Unrecognized or naked identifier '" + e.name +
                             "'. Did you mean 'this." + e.name + "'?");
  }
}

void Resolver::operator()(const ast::ThisExpr& e) {
  if (currSpecName.empty()) {
    throw std::runtime_error(
        "The 'this' keyword can only be used inside a specification block.");
  }
}

void Resolver::operator()(const ast::GroupingExpr& e) {
  if (e.expression) {
    std::visit(std::ref(*this), e.expression->value);
  }
}

void Resolver::operator()(const ast::ListExpr& e) {
  for (const auto& elem : e.elements) {
    if (elem) std::visit(std::ref(*this), elem->value);
  }
}

void Resolver::operator()(const ast::UnaryExpr& e) {
  if (e.operand) {
    std::visit(std::ref(*this), e.operand->value);
  }
}

void Resolver::operator()(const ast::BinaryExpr& e) {
  if (e.left) std::visit(std::ref(*this), e.left->value);
  if (e.right) std::visit(std::ref(*this), e.right->value);
}

void Resolver::operator()(const ast::PostfixExpr& e) {
  if (!e.base) return;

  // Resolve base expr first
  std::visit(std::ref(*this), e.base->value);

  // bool isBaseThis = std::holds_alternative<ast::ThisExpr>(e.base->value);
  const ast::Expr* baseExpr = e.base.get();
  while (baseExpr &&
         std::holds_alternative<ast::GroupingExpr>(baseExpr->value)) {
    const auto& grouping = std::get<ast::GroupingExpr>(baseExpr->value);
    baseExpr = grouping.expression.get();
  }

  const SpecSymbol* activeSpec =
      (baseExpr && std::holds_alternative<ast::ThisExpr>(baseExpr->value))
          ? table.lookup_spec(currSpecName)
          : nullptr;

  for (const auto& op : e.ops) {
    if (std::holds_alternative<ast::MemberAccessOp>(op)) {
      const auto& memOp = std::get<ast::MemberAccessOp>(op);

      // if (isBaseThis) {
      //   // Enforce field validity against st
      //   if (!table.lookup_field(currSpecName, memOp.member)) {
      //     throw std::runtime_error("Specification '" + currSpecName +
      //                              "' has no field named '" + memOp.member +
      //                              "'");
      if (!activeSpec) {
        continue;
      }
      const auto* field = table.lookup_field(activeSpec->name, memOp.member);
      if (!field) {
        throw std::runtime_error("Specification '" + activeSpec->name +
                                 "' has no field named '" + memOp.member + "'");
      }
      // If the field is a custom spec type, allow chained member access to
      // resolve against that spec.
      activeSpec = nullptr;
      if (field->type &&
          std::holds_alternative<ast::SimpleType>(field->type->value)) {
        const auto& st = std::get<ast::SimpleType>(field->type->value);
        if (std::holds_alternative<std::string>(st.value)) {
          activeSpec = table.lookup_spec(std::get<std::string>(st.value));
        }
      }
    } else {
      const auto& idxOp = std::get<ast::IndexOp>(op);
      if (idxOp.index) std::visit(std::ref(*this), idxOp.index->value);
      activeSpec = nullptr;
    }
  }
}