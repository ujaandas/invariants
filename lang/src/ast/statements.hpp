#pragma once

#include <concepts>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

#include "expression.hpp"
#include "helper.hpp"
#include "types.hpp"

namespace invariants::ast {

struct ConstraintStmt {
  ExprPtr expression;
};

inline bool operator==(const ConstraintStmt& a, const ConstraintStmt& b) {
  return ptr_equal(a.expression, b.expression);
}

using ConstraintPtr = std::unique_ptr<ConstraintStmt>;

struct FieldStmt {
  std::string identifier;
  TypePtr type;
  std::vector<ConstraintPtr> constraints;
};

using FieldPtr = std::unique_ptr<FieldStmt>;

inline bool operator==(const FieldStmt& a, const FieldStmt& b) {
  return a.identifier == b.identifier && ptr_equal(a.type, b.type) &&
         ptr_vector_equal(a.constraints, b.constraints);
}

struct InvariantStmt {
  std::string identifier;  // simplified (no need for IdentifierExpr)
  std::vector<ConstraintPtr> constraints;
};

using InvariantPtr = std::unique_ptr<InvariantStmt>;

inline bool operator==(const InvariantStmt& a, const InvariantStmt& b) {
  return a.identifier == b.identifier &&
         ptr_vector_equal(a.constraints, b.constraints);
}

using SpecMember = std::variant<FieldStmt, InvariantStmt>;

struct SpecStmt {
  std::string identifier;
  std::vector<SpecMember> members;
};

inline bool operator==(const SpecStmt& a, const SpecStmt& b) {
  return a.identifier == b.identifier && a.members == b.members;
}

using SpecPtr = std::unique_ptr<SpecStmt>;

struct ModuleStmt {
  std::vector<SpecPtr> specs;
};

using ModulePtr = std::unique_ptr<ModuleStmt>;

inline bool operator==(const ModuleStmt& a, const ModuleStmt& b) {
  return ptr_vector_equal(a.specs, b.specs);
}

struct Stmt {
  using StmtT = std::variant<ConstraintStmt, FieldStmt, InvariantStmt, SpecStmt,
                             ModuleStmt>;

  StmtT value;

  template <typename T>
    requires(!std::same_as<std::decay_t<T>, Stmt>)
  explicit Stmt(T&& v) : value(std::forward<T>(v)) {}

  bool operator==(const Stmt&) const = default;

  friend std::ostream& operator<<(std::ostream& os, const Stmt& expr);
};

}  // namespace invariants::ast