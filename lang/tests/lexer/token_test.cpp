#include "token.hpp"

#include <gtest/gtest.h>

#include <string>

using invariants::lexer::Token;
using invariants::lexer::TokenType;

TEST(TokenToStringTest, UsesLexemeAndStringLiteral) {
  const Token token(TokenType::LIT_STRING, "\"hello\"", std::string("hello"),
                    1);

  const std::string expected =
      std::to_string(static_cast<int>(TokenType::LIT_STRING)) +
      " \"hello\" hello";
  EXPECT_EQ(token.toString(), expected);
}

TEST(TokenToStringTest, FormatsNumberLiteral) {
  const Token token(TokenType::LIT_NUMBER, "3.5", 3.5, 2);

  const std::string expected =
      std::to_string(static_cast<int>(TokenType::LIT_NUMBER)) + " 3.5 3.500000";
  EXPECT_EQ(token.toString(), expected);
}

TEST(TokenToStringTest, FormatsBooleanLiterals) {
  const Token trueToken(TokenType::LIT_BOOLEAN, "true", true, 3);
  const Token falseToken(TokenType::LIT_BOOLEAN, "false", false, 4);

  const std::string trueExpected =
      std::to_string(static_cast<int>(TokenType::LIT_BOOLEAN)) + " true true";
  const std::string falseExpected =
      std::to_string(static_cast<int>(TokenType::LIT_BOOLEAN)) + " false false";

  EXPECT_EQ(trueToken.toString(), trueExpected);
  EXPECT_EQ(falseToken.toString(), falseExpected);
}

TEST(TokenToStringTest, FormatsNullLiteralAsNil) {
  const Token token(TokenType::LIT_NULL, "null", std::monostate{}, 5);

  const std::string expected =
      std::to_string(static_cast<int>(TokenType::LIT_NULL)) + " null nil";
  EXPECT_EQ(token.toString(), expected);
}
