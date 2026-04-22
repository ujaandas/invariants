#include "lexer.hpp"

#include <cctype>
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

  // Lookahead + 1 without consuming
  auto peekNext = [this]() {
    if (curr + 1 >= source.length()) return '\0';
    return source[curr + 1];
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

    // Handle string literals
    case '"': {
      while (peek() != '"' && curr < source.length()) {
        if (peek() == '\n') line++;
        advance();
      }

      if (curr >= source.length()) {
        // TODO: replace with proper error handling
        throw std::range_error("unterminated string");
      }

      // Consume closing quote
      advance();
      auto value = source.substr(start + 1, curr - start - 2);
      addToken(TokenType::LIT_STRING, value);

      break;
    }

    default:
      // Can we clean this up? Put it into its own case?
      if (std::isdigit(c)) {
        while (std::isdigit(peek())) advance();

        if (peek() == '.' && std::isdigit(peekNext())) {
          advance();
          while (std::isdigit(peek())) advance();
        }

        addToken(TokenType::LIT_NUMBER,
                 std::stod(source.substr(start, curr - start)));
      } else {
        // TODO: replace this with proper error handling
        throw std::invalid_argument("received invalid token");
      }
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
