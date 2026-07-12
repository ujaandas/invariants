#pragma once

#include "ast_expr.hpp"

namespace invariants::ir {

struct FieldInvariant {
  ast::LiteralExpr what;
};

}  // namespace invariants::ir