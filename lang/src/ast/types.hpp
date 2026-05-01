#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <variant>
namespace invariants::ast {

struct Type {
  virtual ~Type() = default;
};

using TypePtr = std::unique_ptr<Type>;

enum class BuiltinType : std::uint8_t { Number, Integer, String, Boolean };

struct SimpleType : Type {
  std::variant<BuiltinType, std::string> value;
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