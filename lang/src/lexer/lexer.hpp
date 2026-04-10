#pragma once

#include <string>
#include <vector>

#include "token.hpp"

namespace invariants::lexer {

class Lexer {
 private:
  const std::string source;
  std::vector<Token> tokens;
  size_t start = 0;
  size_t curr = 0;
  size_t line = 1;

  void scanToken();
  char advance();
  void addToken(TokenType type);
  void addToken(TokenType type, Literal literal);

 public:
  explicit Lexer(std::string source);
  std::vector<Token> scanTokens();
};

}  // namespace invariants::lexer