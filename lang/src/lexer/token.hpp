#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <variant>

namespace invariants::lexer {

enum class TokenType : std::uint8_t {
  // Single-character tokens
  LEFT_BRACKET,   // [
  RIGHT_BRACKET,  // ]
  LEFT_BRACE,     // {
  RIGHT_BRACE,    // }
  COLON,          // :
  COMMA,          // ,
  DOT,            // .
  SEMICOLON,      // ;
  PLUS,           // +
  SLASH,          // /
  STAR,           // *
  PERCENTAGE,     // %

  // Possibly multiple-character tokens
  BANG,           // !
  BANG_EQUAL,     // !=
  EQUAL,          // =
  EQUAL_EQUAL,    // ==
  GREATER,        // >
  GREATER_EQUAL,  // >=
  LESS,           // <
  LESS_EQUAL,     // <=
  MINUS,          // -
  ARROW,          // ->

  // Literals
  LIT_IDENTIFIER,  // FooBar
  LIT_STRING,      // "hello there"
  LIT_NUMBER,      // 6.7
  LIT_INTEGER,     // 67
  LIT_BOOLEAN,     // true/false
  LIT_NULL,        // null

  // Structural keywords
  KW_SPEC,       // spec
  KW_FIELD,      // field
  KW_CHECK,      // check
  KW_INVARIANT,  // invariant

  // Type keywords
  KW_BOOLEAN,  // Boolean
  KW_ARRAY,    // Array<Foo>
  KW_NULL,     // Null
  KW_STRING,   // String
  KW_NUMBER,   // Number
  KW_INTEGER,  // Integer

  // Operator keywords
  KW_IN,        // IN
  KW_NOT_IN,    // NIN
  KW_CONTAINS,  // NI

  EOF_TOKEN,
};

using Literal = std::variant<std::monostate,  // null
                             std::string,     // identifiers + strings
                             double,          // numbers
                             bool             // booleans
                             >;

class Token {
 private:
  TokenType type;
  std::string lexeme;
  Literal literal;
  std::size_t line;

 public:
  Token(TokenType type, std::string lexeme, Literal literal, std::size_t line);
  std::string toString() const;
};

}  // namespace invariants::lexer