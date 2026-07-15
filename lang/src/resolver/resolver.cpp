#include "resolver.hpp"

#include <memory>
#include <stdexcept>

#include "statements.hpp"
#include "symbol_table.hpp"

using namespace invariants::resolver;

void Resolver::operator()(const ast::ModuleStmt& e) {
  for (const auto& spec : e.specs) {
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

  for (const auto& constraint : e.constraints) {
    if (constraint) {
      (*this)(*constraint);
    }
  }
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

// TODO: Placeholders for linker
void Resolver::operator()(const ast::LiteralExpr& e) {}
void Resolver::operator()(const ast::IdentifierExpr& e) {}
void Resolver::operator()(const ast::ThisExpr&) {}
void Resolver::operator()(const ast::ListExpr& e) {}
void Resolver::operator()(const ast::GroupingExpr& e) {}
void Resolver::operator()(const ast::PostfixExpr& e) {}
void Resolver::operator()(const ast::MemberAccessOp& e) {}
void Resolver::operator()(const ast::IndexOp& e) {}
void Resolver::operator()(const ast::UnaryExpr& e) {}
void Resolver::operator()(const ast::BinaryExpr& e) {}