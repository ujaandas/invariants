#pragma once

#include <cstdint>
#include <memory>

#include "expression.hpp"

namespace invariants::ast {

struct Type : Node {
  virtual ~Type() = default;
};

using TypePtr = std::unique_ptr<Type>;

enum class BuiltinType : std::uint8_t { Number, Integer, String, Boolean };

struct NamedType : Type {
  std::string name;
};

struct SimpleType : Type {
  BuiltinType builtin;
};

// TODO: define constructors with std::move for Array and Map types
struct ArrayType : Type {
  TypePtr element;
};

struct MapType : Type {
  TypePtr key;
  TypePtr value;
};

}  // namespace invariants::ast