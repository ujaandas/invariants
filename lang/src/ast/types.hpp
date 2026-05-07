#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <variant>

#include "helper.hpp"

namespace invariants::ast {

// Forward decl
struct Type;
using TypePtr = std::unique_ptr<Type>;

enum class BuiltinType : std::uint8_t { Number, Integer, String, Boolean };

struct SimpleType {
  std::variant<BuiltinType, std::string> value;
  bool operator==(const SimpleType&) const = default;
};

struct ArrayType {
  TypePtr element;
};
inline bool operator==(const ArrayType& a, const ArrayType& b) {
  return ptr_equal(a.element, b.element);
}

struct MapType {
  TypePtr key;
  TypePtr value;
};
inline bool operator==(const MapType& a, const MapType& b) {
  return ptr_equal(a.key, b.key) && ptr_equal(a.value, b.value);
}

struct Type {
  using TypeT = std::variant<SimpleType, ArrayType, MapType>;

  TypeT value;

  template <typename T>
    requires(!std::same_as<std::decay_t<T>, Type>)
  explicit Type(T&& v) : value(std::forward<T>(v)) {}

  bool operator==(const Type&) const = default;
};

}  // namespace invariants::ast