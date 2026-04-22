#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "token.hpp"

namespace invariants::lexer {

class Lexer {
 private:
  const std::string source;
  std::vector<Token> tokens;
  std::unordered_map<std::string, TokenType> keywords{
      {"spec", TokenType::KW_SPEC},
      {"field", TokenType::KW_FIELD},
      {"check", TokenType::KW_CHECK},
      {"invariant", TokenType::KW_INVARIANT},
      {"Boolean", TokenType::KW_BOOLEAN},
      {"true", TokenType::LIT_BOOLEAN_T},
      {"false", TokenType::LIT_BOOLEAN_F},
      {"Array", TokenType::KW_ARRAY},
      {"Null", TokenType::KW_NULL},
      {"null", TokenType::LIT_NULL},
      {"String", TokenType::KW_STRING},
      {"Number", TokenType::KW_NUMBER},
      {"Integer", TokenType::KW_INTEGER},
      {"IN", TokenType::KW_IN},
      {"NIN", TokenType::KW_NOT_IN},
      {"NI", TokenType::KW_CONTAINS},
  };
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