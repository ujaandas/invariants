#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <variant>

namespace invariants::ast {

// Forward decl
struct Type;
using TypePtr = std::unique_ptr<Type>;

enum class BuiltinType : std::uint8_t { Number, Integer, String, Boolean };

struct SimpleType {
  std::variant<BuiltinType, std::string> value;
};

struct ArrayType {
  TypePtr element;
};

struct MapType {
  TypePtr key;
  TypePtr value;
};

struct Type {
  using TypeT = std::variant<SimpleType, ArrayType, MapType>;

  TypeT value;

  template <typename T>
  explicit Type(T v) : value(std::move(v)) {}
};

}  // namespace invariants::ast