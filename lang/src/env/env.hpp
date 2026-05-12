#pragma once

#include <string>
#include <unordered_map>

#include "token.hpp"

namespace invariants::env {

using ObjT = int;

class Environment {
 private:
  std::unordered_map<std::string, ObjT> values;

  void define(std::string name, ObjT value);
  ObjT get(const lexer::Token& token);
};

}  // namespace invariants::env