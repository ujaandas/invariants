#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

#include "types.hpp"

namespace invariants::binder {

// using ModuleId = std::uint32_t; // TODO: Add importing module support later

using SpecId = std::uint32_t;
using FieldId = std::uint32_t;

struct SpecSymbol;

using TypeVar = std::variant<ast::BuiltinType, const SpecSymbol*>;

struct ResolvedType {
  TypeVar type;
  bool isBuiltin() const;
  bool isCustom() const;
};

struct FieldSymbol {
  FieldId id;
  std::string name;
  ResolvedType resType;
};

struct SpecSymbol {
  SpecId id;
  std::string name;
  std::unordered_map<std::string, std::unique_ptr<FieldSymbol>,
                     std::hash<std::string_view>, std::equal_to<>>
      fields;
};

class SymbolTable {
 public:
  const SpecSymbol* lookup_spec(std::string_view) const;
  const FieldSymbol* lookup_field(std::string_view, std::string_view) const;

  SpecSymbol* add_spec(std::string_view, std::unique_ptr<SpecSymbol>);
  FieldSymbol* add_field(std::string_view, std::string_view,
                         std::unique_ptr<FieldSymbol>);

  std::size_t get_total_field_count() const;

 private:
  SpecId nextSpecId = 0;
  FieldId nextFieldId = 0;

  std::unordered_map<std::string, std::unique_ptr<SpecSymbol>,
                     std::hash<std::string_view>, std::equal_to<>>
      specs;
};

}  // namespace invariants::binder