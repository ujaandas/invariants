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
  // TODO: Implement field resolution logic (adding to symbol table, etc.)
}

void Resolver::operator()(const ast::InvariantStmt& e) {
  // TODO: Implement invariant resolution logic
}

void Resolver::operator()(const ast::ConstraintStmt& e) {
  // TODO: Implement constraint resolution logic
}