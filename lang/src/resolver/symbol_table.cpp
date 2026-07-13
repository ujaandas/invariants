#include "symbol_table.hpp"

#include <string>

using namespace invariants::resolver;

const SpecSymbol* SymbolTable::lookup_spec(std::string_view spec) {
  auto it = specs.find(std::string(spec));
  if (it != specs.end()) {
    return it->second.get();
  }
  return nullptr;
}

const FieldSymbol* SymbolTable::lookup_field(std::string_view spec_name,
                                             std::string_view field_name) {
  auto foundSpec = lookup_spec(spec_name);
  if (!foundSpec) {
    return nullptr;
  }

  auto& fields = foundSpec->fields;
  auto it = fields.find(std::string(field_name));
  if (it != fields.end()) {
    return it->second.get();
  }
  return nullptr;
}