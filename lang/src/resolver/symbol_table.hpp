#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

#include "statements.hpp"
#include "types.hpp"

namespace invariants::resolver {

// using ModuleId = std::uint32_t; // TODO: Add importing module support later

using SpecId = std::uint32_t;
using FieldId = std::uint32_t;

struct FieldSymbol {
  FieldId id;
  std::string name;
  ast::Type type;

  const ast::FieldStmt* decl;
};

struct SpecSymbol {
  SpecId id;
  std::string name;

  const ast::SpecStmt* decl;

  std::unordered_map<std::string, std::unique_ptr<FieldSymbol>> fields;
};

class SymbolTable {
 public:
  const SpecSymbol* lookup_spec(std::string_view);
  const FieldSymbol* lookup_field(std::string_view, std::string_view);

 private:
  std::unordered_map<std::string, std::unique_ptr<SpecSymbol>> specs;
};

}  // namespace invariants::resolver