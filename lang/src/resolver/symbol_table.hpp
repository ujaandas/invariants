#pragma once

#include <cstdint>
#include <string>

#include "statements.hpp"
#include "types.hpp"

namespace invariants::resolver {

// using ModuleId = std::uint32_t; // TODO: Add importing module support later

using SpecId = std::uint32_t;
struct SpecSymbol {
  SpecId id;
  std::string name;
  const ast::SpecStmt* decl;
};

using FieldId = std::uint32_t;
struct FieldSymbol {
  FieldId id;
  std::string name;

  ast::Type type;
  const ast::FieldStmt* decl;
};

using ConstraintId = std::uint32_t;
struct ConstraintSymbol {
  ConstraintId id;
  const ast::ConstraintStmt* decl;
};

}  // namespace invariants::resolver