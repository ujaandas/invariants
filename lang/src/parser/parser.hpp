#pragma once

#include <vector>

#include "token.hpp"

namespace invariants::parser {

class Parser {
 private:
  int curr = 0;
  std::vector<lexer::Token> tokens;

 public:
  explicit Parser(const std::vector<lexer::Token>& tokens);
};

}  // namespace invariants::parser