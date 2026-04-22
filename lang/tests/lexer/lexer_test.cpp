#include "lexer.hpp"

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <vector>

#include "token.hpp"

using invariants::lexer::Lexer;
using invariants::lexer::Token;
using invariants::lexer::TokenType;

// TODO: add tests for each token type

TEST(LexerTest, ScansSingleBracket) {
  Lexer lexer("[");

  const auto tokens = lexer.scanTokens();

  EXPECT_EQ(tokens.size(), 2);
  EXPECT_EQ(tokens[0], Token(TokenType::LEFT_BRACKET, "[", 1));
  EXPECT_EQ(tokens[1], Token(TokenType::EOF_TOKEN, "", 1));
}

TEST(LexerTest, ScansSimplePunctuationSequence) {
  Lexer lexer("[]+;");

  const auto tokens = lexer.scanTokens();

  const std::vector<Token> expected = {
      Token(TokenType::LEFT_BRACKET, "[", 1),
      Token(TokenType::RIGHT_BRACKET, "]", 1),
      Token(TokenType::PLUS, "+", 1),
      Token(TokenType::SEMICOLON, ";", 1),
      Token(TokenType::EOF_TOKEN, "", 1),
  };

  EXPECT_EQ(tokens, expected);
}

TEST(LexerTest, ThrowsOnInvalidToken) {
  Lexer lexer("@");
  EXPECT_THROW(lexer.scanTokens(), std::invalid_argument);
}

TEST(LexerTest, ScansSingleBang) {
  Lexer lexer("!");

  const auto tokens = lexer.scanTokens();

  EXPECT_EQ(tokens.size(), 2);
  EXPECT_EQ(tokens[0], Token(TokenType::BANG, "!", 1));
  EXPECT_EQ(tokens[1], Token(TokenType::EOF_TOKEN, "", 1));
}

TEST(LexerTest, ScansBangEqual) {
  Lexer lexer("!=");

  const auto tokens = lexer.scanTokens();

  EXPECT_EQ(tokens.size(), 2);
  EXPECT_EQ(tokens[0], Token(TokenType::BANG_EQUAL, "!=", 1));
  EXPECT_EQ(tokens[1], Token(TokenType::EOF_TOKEN, "", 1));
}

TEST(LexerTest, ScansSingleDivide) {
  Lexer lexer("/");

  const auto tokens = lexer.scanTokens();

  EXPECT_EQ(tokens.size(), 2);
  EXPECT_EQ(tokens[0], Token(TokenType::SLASH, "/", 1));
  EXPECT_EQ(tokens[1], Token(TokenType::EOF_TOKEN, "", 1));
}

TEST(LexerTest, ScansComment) {
  Lexer lexer("// this is a comment");

  const auto tokens = lexer.scanTokens();

  EXPECT_EQ(tokens.size(), 1);
  EXPECT_EQ(tokens[0], Token(TokenType::EOF_TOKEN, "", 1));
}

TEST(LexerTest, ScansIgnoresWhitespace) {
  Lexer lexer(" ");

  const auto tokens = lexer.scanTokens();

  EXPECT_EQ(tokens.size(), 1);
  EXPECT_EQ(tokens[0], Token(TokenType::EOF_TOKEN, "", 1));
}

TEST(LexerTest, ScansNewline) {
  Lexer lexer("\n");

  const auto tokens = lexer.scanTokens();

  EXPECT_EQ(tokens.size(), 1);
  EXPECT_EQ(tokens[0], Token(TokenType::EOF_TOKEN, "", 2));
}

TEST(LexerTest, ScansMultipleCommentedLines) {
  Lexer lexer(
      "// this is a comment\n"
      "// this is another comment\n");

  const auto tokens = lexer.scanTokens();

  EXPECT_EQ(tokens.size(), 1);
  EXPECT_EQ(tokens[0], Token(TokenType::EOF_TOKEN, "", 3));
}

TEST(LexerTest, ScansDivideAfterComments) {
  Lexer lexer(
      "// this is a comment\n"
      "// this is another comment\n"
      "/");

  const auto tokens = lexer.scanTokens();

  EXPECT_EQ(tokens.size(), 2);
  EXPECT_EQ(tokens[0], Token(TokenType::SLASH, "/", 3));
  EXPECT_EQ(tokens[1], Token(TokenType::EOF_TOKEN, "", 3));
}

TEST(LexerTest, ScansStringLiteral) {
  Lexer lexer("\"Hello, World!\"");

  const auto tokens = lexer.scanTokens();

  EXPECT_EQ(tokens.size(), 2);
  EXPECT_EQ(tokens[0], Token(TokenType::LIT_STRING, "\"Hello, World!\"",
                             "Hello, World!", 1));
  EXPECT_EQ(tokens[1], Token(TokenType::EOF_TOKEN, "", 1));
}

TEST(LexerTest, ScansIntegerLiteral) {
  Lexer lexer("123");

  const auto tokens = lexer.scanTokens();

  EXPECT_EQ(tokens.size(), 2);
  EXPECT_EQ(tokens[0], Token(TokenType::LIT_INTEGER, "123", 123, 1));
  EXPECT_EQ(tokens[1], Token(TokenType::EOF_TOKEN, "", 1));
}

TEST(LexerTest, ScansDoubleLiteral) {
  Lexer lexer("123.456");

  const auto tokens = lexer.scanTokens();

  EXPECT_EQ(tokens.size(), 2);
  EXPECT_EQ(tokens[0], Token(TokenType::LIT_NUMBER, "123.456", 123.456, 1));
  EXPECT_EQ(tokens[1], Token(TokenType::EOF_TOKEN, "", 1));
}

// TEST(LexerTest, LeadingDecimalIllegal) {
//   Lexer lexer(".456");

//   const auto tokens = lexer.scanTokens();

//   for (auto& token : tokens) {
//     std::cout << token.toString() << ",\n";
//   }

//   // .456 actually evaluates to tokens DOT, 456.000..., EOF
//   EXPECT_EQ(tokens.size(), 3);
//   EXPECT_EQ(tokens[0], Token(TokenType::LIT_NUMBER, ".456", .456, 1));
//   EXPECT_EQ(tokens[1], Token(TokenType::EOF_TOKEN, "", 1));
// }

TEST(LexerTest, ScansIdentifier) {
  Lexer lexer("foobar");

  const auto tokens = lexer.scanTokens();

  EXPECT_EQ(tokens.size(), 2);
  EXPECT_EQ(tokens[0], Token(TokenType::LIT_IDENTIFIER, "foobar", 1));
  EXPECT_EQ(tokens[1], Token(TokenType::EOF_TOKEN, "", 1));
}

TEST(LexerTest, ScansSpecKeyword) {
  Lexer lexer("spec");

  const auto tokens = lexer.scanTokens();

  EXPECT_EQ(tokens.size(), 2);
  EXPECT_EQ(tokens[0], Token(TokenType::KW_SPEC, "spec", 1));
  EXPECT_EQ(tokens[1], Token(TokenType::EOF_TOKEN, "", 1));
}

TEST(LexerTest, ScansBooleanKeyword) {
  Lexer lexer("Boolean");

  const auto tokens = lexer.scanTokens();

  EXPECT_EQ(tokens.size(), 2);
  EXPECT_EQ(tokens[0], Token(TokenType::KW_BOOLEAN, "Boolean", 1));
  EXPECT_EQ(tokens[1], Token(TokenType::EOF_TOKEN, "", 1));
}

TEST(LexerTest, DifferentiatesKeywordLiteralBooleanT) {
  Lexer lexer("Boolean true");

  const auto tokens = lexer.scanTokens();

  EXPECT_EQ(tokens.size(), 3);
  EXPECT_EQ(tokens[0], Token(TokenType::KW_BOOLEAN, "Boolean", 1));
  EXPECT_EQ(tokens[1], Token(TokenType::LIT_BOOLEAN_T, "true", true, 1));
  EXPECT_EQ(tokens[2], Token(TokenType::EOF_TOKEN, "", 1));
}

TEST(LexerTest, DifferentiatesKeywordLiteralBooleanF) {
  Lexer lexer("Boolean false");

  const auto tokens = lexer.scanTokens();

  EXPECT_EQ(tokens.size(), 3);
  EXPECT_EQ(tokens[0], Token(TokenType::KW_BOOLEAN, "Boolean", 1));
  EXPECT_EQ(tokens[1], Token(TokenType::LIT_BOOLEAN_F, "false", false, 1));
  EXPECT_EQ(tokens[2], Token(TokenType::EOF_TOKEN, "", 1));
}