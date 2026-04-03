#pragma once

#include <cstdint>

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
  LIT_STRING,      // a-z, A-Z
  LIT_NUMBER,      // 0-9

  // Structural keywords
  KW_SPEC,       // spec
  KW_FIELD,      // field
  KW_CHECK,      // check
  KW_INVARIANT,  // invariant

  // Type keywords
  KW_BOOLEAN,  // true/false
  KW_ARRAY,    // Array<Foo>
  KW_NULL,     // null
  KW_STRING,   // "foobar"
  KW_NUMBER,   // 6.7
  KW_INTEGER,  // 42

  // Operator keywords
  KW_IN,        // IN
  KW_NOT_IN,    // NIN
  KW_CONTAINS,  // NI

  EOF
};