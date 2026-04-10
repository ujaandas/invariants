#include "lexer.hpp"

#include <stdexcept>
#include <utility>
#include <variant>

#include "token.hpp"

using namespace ::invariants::lexer;

Lexer::Lexer(std::string source)
    : source(std::move(source)), start(0), curr(0), line(1) {}

char Lexer::advance() { return source[curr++]; }

void Lexer::addToken(TokenType type) { addToken(type, std::monostate{}); }

void Lexer::addToken(TokenType type, Literal literal) {
  tokens.push_back(
      Token(type, source.substr(start, curr - start), literal, line));
}

void Lexer::scanToken() {
  char c = advance();

  switch (c) {
    case '[':
      addToken(TokenType::LEFT_BRACKET);
      break;
    case ']':
      addToken(TokenType::RIGHT_BRACKET);
      break;
    case '(':
      addToken(TokenType::LEFT_BRACE);
      break;
    case ')':
      addToken(TokenType::RIGHT_BRACE);
      break;
    case ':':
      addToken(TokenType::COLON);
      break;
    case ',':
      addToken(TokenType::COMMA);
      break;
    case '.':
      addToken(TokenType::DOT);
      break;
    case ';':
      addToken(TokenType::SEMICOLON);
      break;
    case '+':
      addToken(TokenType::PLUS);
      break;
    case '/':
      addToken(TokenType::SLASH);
      break;
    case '*':
      addToken(TokenType::STAR);
      break;
    case '%':
      addToken(TokenType::PERCENTAGE);
      break;
    default:
      // TODO: replace this with proper error handling
      throw std::invalid_argument("received invalid token");
  }
}

std::vector<Token> Lexer::scanTokens() {
  while (curr < source.length()) {
    start = curr;
    scanToken();
  }

  tokens.push_back(Token(TokenType::EOF_TOKEN, "", std::monostate{}, line));

  return tokens;
}
