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

// TEST(LexerTest, ScansMultilineCommentAndDivide) {
//   Lexer lexer(
//       "// this is a comment\n"
//       "// this is another comment\n"
//       "/");

//   const auto tokens = stringify(lexer.scanTokens());

//   const std::vector<std::string> expected = {
//       std::to_string(static_cast<int>(TokenType::SLASH)) + " / nil",
//       std::to_string(static_cast<int>(TokenType::EOF_TOKEN)) + "  nil",
//   };

//   EXPECT_EQ(tokens, expected);
// }