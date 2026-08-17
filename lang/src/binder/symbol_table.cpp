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

bool SymbolTable::add_spec(std::string_view name,
                           std::unique_ptr<SpecSymbol> spec) {
  if (!spec) return false;

  auto [_, inserted] = specs.try_emplace(std::string(name), std::move(spec));
  return inserted;
}

bool SymbolTable::add_field(std::string_view specName,
                            std::string_view fieldName,
                            std::unique_ptr<FieldSymbol> field) {
  if (!field) return false;

  auto it = specs.find(std::string(specName));
  if (it == specs.end()) {
    return false;
  }

  auto& fields = it->second->fields;
  auto [_, inserted] =
      fields.try_emplace(std::string(fieldName), std::move(field));
  return inserted;
}