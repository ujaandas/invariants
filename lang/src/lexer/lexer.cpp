#include "lexer.hpp"

#include <stdexcept>
#include <utility>
#include <variant>

#include "token.hpp"

using namespace ::invariants::lexer;

Lexer::Lexer(std::string source) : source(std::move(source)) {}

char Lexer::advance() { return source[curr++]; }

void Lexer::addToken(TokenType type) { addToken(type, std::monostate{}); }

void Lexer::addToken(TokenType type, Literal literal) {
  tokens.push_back(
      Token(type, source.substr(start, curr - start), literal, line));
}

void Lexer::scanToken() {
  char c = advance();

  // Only consume the next token if its equal to what we expected
  auto match = [this](char expected) {
    if (curr >= source.length()) return false;
    if (source[curr] != expected) return false;

    advance();

    return true;
  };

  // Lookahead without consuming
  auto peek = [this]() {
    if (curr >= source.length()) return '\0';
    return source[curr];
  };

  switch (c) {
    // Single-character tokens
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
    // case '/':
    //   addToken(TokenType::SLASH);
    //   break;
    case '*':
      addToken(TokenType::STAR);
      break;
    case '%':
      addToken(TokenType::PERCENTAGE);
      break;

    // Operators/possibly multi-char tokens
    case '!':
      addToken(match('=') ? TokenType::BANG_EQUAL : TokenType::BANG);
      break;
    case '=':
      addToken(match('=') ? TokenType::EQUAL_EQUAL : TokenType::EQUAL);
      break;
    case '>':
      addToken(match('=') ? TokenType::GREATER_EQUAL : TokenType::GREATER);
      break;
    case '<':
      addToken(match('=') ? TokenType::LESS_EQUAL : TokenType::LESS);
      break;
    case '-':
      addToken(match('>') ? TokenType::ARROW : TokenType::MINUS);
      break;

    // Comment or divide?
    case '/':
      if (match('/')) {
        // If next char is also '/', consume comment
        while (peek() != '\n' && curr < source.length()) {
          advance();
        }
      } else {
        // Otherwise it's just a slash token
        addToken(TokenType::SLASH);
      }
      break;

    // Handle newline/whitespace
    case ' ':
      break;
    case '\r':
      break;
    case '\t':
      break;

    case '\n':
      line++;
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
