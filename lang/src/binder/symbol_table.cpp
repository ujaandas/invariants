#include "symbol_table.hpp"

#include <memory>
#include <string>
#include <utility>

using namespace invariants::binder;

const SpecSymbol* SymbolTable::lookup_spec(std::string_view spec) const {
  auto it = specs.find(spec);
  if (it != specs.end()) {
    return it->second.get();
  }
  return nullptr;
}

const FieldSymbol* SymbolTable::lookup_field(std::string_view specName,
                                             std::string_view fieldName) const {
  auto foundSpec = lookup_spec(specName);
  if (!foundSpec) {
    return nullptr;
  }

  auto& fields = foundSpec->fields;
  auto it = fields.find(fieldName);
  if (it != fields.end()) {
    return it->second.get();
  }
  return nullptr;
}

SpecSymbol* SymbolTable::add_spec(std::string_view name,
                                  const ast::SpecStmt* decl) {
  if (specs.contains(name)) return nullptr;

  auto spec = std::make_unique<SpecSymbol>();
  spec->id = nextSpecId++;
  spec->name = std::string(name);

  auto* rawPtr = spec.get();
  specs.emplace(std::string(name), std::move(spec));
  return rawPtr;
}

FieldSymbol* SymbolTable::add_field(std::string_view specName,
                                    std::string_view fieldName,
                                    ResolvedType type,
                                    const ast::FieldStmt* decl) {
  auto it = specs.find(specName);
  if (it == specs.end()) return nullptr;

  auto& fields = it->second->fields;
  if (fields.contains(fieldName)) return nullptr;

  auto field = std::make_unique<FieldSymbol>();
  field->id = nextFieldId++;
  field->name = std::string(fieldName);
  field->resType = std::move(type);

  auto* rawPtr = field.get();
  fields.emplace(std::string(fieldName), std::move(field));
  return rawPtr;
}