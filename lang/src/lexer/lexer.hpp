#pragma once

#include <string>
#include <vector>

#include "token.hpp"

namespace invariants::lexer {

class Lexer {
 private:
  const std::string source;
  std::vector<Token> tokens;
  size_t start;
  size_t curr;
  size_t line;

  void scanToken();
  char advance();
  void addToken(TokenType type);
  void addToken(TokenType type, Literal literal);

 public:
  explicit Lexer(std::string source);
  std::vector<Token> scanTokens();
};

}  // namespace invariants::lexer