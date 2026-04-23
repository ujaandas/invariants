#include "token.hpp"

#include <gtest/gtest.h>

#include <string>
#include <variant>

using invariants::lexer::Token;
using invariants::lexer::TokenType;

TEST(TokenTest, NoLiteralCtorMatch) {
  const Token token1(TokenType::EOF_TOKEN, "", 1);
  const Token token2(TokenType::EOF_TOKEN, "", std::monostate{}, 1);

  EXPECT_EQ(token1, token2);
}

TEST(TokenTest, EqualWhenAllFieldsMatch) {
  const Token token1(TokenType::LIT_STRING, "\"hello\"", std::string("hello"),
                     1);
  const Token token2(TokenType::LIT_STRING, "\"hello\"", std::string("hello"),
                     1);

  EXPECT_EQ(token1, token2);
}

TEST(TokenTest, NotEqualWhenTypeDiffers) {
  const Token token1(TokenType::LIT_STRING, "\"hello\"", std::string("hello"),
                     1);
  const Token token2(TokenType::LIT_NUMBER, "\"hello\"", std::string("hello"),
                     1);

  EXPECT_NE(token1, token2);
}

TEST(TokenTest, NotEqualWhenLexemeDiffers) {
  Token a(TokenType::LIT_STRING, "\"hello\"", std::string("hello"), 1);
  Token b(TokenType::LIT_STRING, "\"world\"", std::string("hello"), 1);

  EXPECT_NE(a, b);
}

TEST(TokenTest, NotEqualWhenLiteralDiffers) {
  Token a(TokenType::LIT_STRING, "\"hello\"", std::string("hello"), 1);
  Token b(TokenType::LIT_STRING, "\"hello\"", std::string("world"), 1);

  EXPECT_NE(a, b);
}

TEST(TokenTest, NotEqualWhenLineDiffers) {
  Token a(TokenType::LIT_STRING, "\"hello\"", std::string("hello"), 1);
  Token b(TokenType::LIT_STRING, "\"hello\"", std::string("hello"), 2);

  EXPECT_NE(a, b);
}

TEST(TokenTest, FormatsStringLiteral) {
  const Token token(TokenType::LIT_STRING, "\"hello\"", std::string("hello"),
                    1);

  const std::string expected =
      std::to_string(static_cast<int>(TokenType::LIT_STRING)) +
      " \"hello\" hello";
  EXPECT_EQ(token.toString(), expected);
}

TEST(TokenTest, FormatsNumberLiteral) {
  const Token token(TokenType::LIT_NUMBER, "3.5", 3.5, 2);

  const std::string expected =
      std::to_string(static_cast<int>(TokenType::LIT_NUMBER)) + " 3.5 3.500000";
  EXPECT_EQ(token.toString(), expected);
}

TEST(TokenTest, FormatsBooleanLiterals) {
  const Token trueToken(TokenType::LIT_BOOLEAN_T, "true", true, 3);
  const Token falseToken(TokenType::LIT_BOOLEAN_F, "false", false, 4);

  const std::string trueExpected =
      std::to_string(static_cast<int>(TokenType::LIT_BOOLEAN_T)) + " true true";
  const std::string falseExpected =
      std::to_string(static_cast<int>(TokenType::LIT_BOOLEAN_F)) +
      " false false";

  EXPECT_EQ(trueToken.toString(), trueExpected);
  EXPECT_EQ(falseToken.toString(), falseExpected);
}

TEST(TokenTest, FormatsNullLiteralAsNil) {
  const Token token(TokenType::LIT_NULL, "null", std::monostate{}, 5);

  const std::string expected =
      std::to_string(static_cast<int>(TokenType::LIT_NULL)) + " null null";
  EXPECT_EQ(token.toString(), expected);
}
