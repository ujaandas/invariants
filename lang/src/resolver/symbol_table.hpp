#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>

#include "statements.hpp"
#include "types.hpp"

namespace invariants::resolver {

// using ModuleId = std::uint32_t; // TODO: Add importing module support later

using SpecId = std::uint32_t;
using FieldId = std::uint32_t;
using ConstraintId = std::uint32_t;

struct ConstraintSymbol {
  ConstraintId id;
  const ast::ConstraintStmt* decl;
};

struct FieldSymbol {
  FieldId id;
  std::string name;
  ast::Type type;

  const ast::FieldStmt* decl;

  std::vector<ConstraintSymbol> constraints;
};

struct SpecSymbol {
  SpecId id;
  std::string name;

  const ast::SpecStmt* decl;

  std::unordered_map<std::string, FieldSymbol> fields;
};

class SymbolTable {
 public:
  explicit SymbolTable() = default;
  SpecSymbol* lookup_spec(std::string_view);

 private:
  std::unordered_map<std::string, SpecSymbol> specs;
};

}  // namespace invariants::resolver