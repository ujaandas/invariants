#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace invariants::runtime {

struct ArrayValue;

using Value =
    std::variant<std::monostate,  // Null or empty state
                 bool, int, double, std::string, std::shared_ptr<ArrayValue> >;

struct ArrayValue {
  std::vector<Value> elements;
};

using Environment = std::unordered_map<std::string, Value>;

}  // namespace invariants::runtime