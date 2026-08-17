#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>

#include "statements.hpp"
#include "types.hpp"

struct TransparentStringHash {
  using is_transparent = void;

  std::size_t operator()(std::string_view sv) const noexcept {
    return std::hash<std::string_view>{}(sv);
  }
};

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
                     TransparentStringHash, std::equal_to<>>
      fields;
};

class SymbolTable {
 public:
  const SpecSymbol* lookup_spec(std::string_view) const;
  const FieldSymbol* lookup_field(std::string_view, std::string_view) const;

  SpecSymbol* add_spec(std::string_view, const ast::SpecStmt*);
  FieldSymbol* add_field(std::string_view, std::string_view, ResolvedType,
                         const ast::FieldStmt*);

  std::size_t get_total_field_count() const;

 private:
  SpecId nextSpecId = 0;
  FieldId nextFieldId = 0;

  std::unordered_map<std::string, std::unique_ptr<SpecSymbol>,
                     TransparentStringHash, std::equal_to<>>
      specs;
};

}  // namespace invariants::binder