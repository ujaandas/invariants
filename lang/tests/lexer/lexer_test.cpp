#include "lexer.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

#include "token.hpp"

using invariants::lexer::Lexer;
using invariants::lexer::Token;
using invariants::lexer::TokenType;

namespace {

std::vector<std::string> stringify(const std::vector<Token>& tokens) {
  std::vector<std::string> out(tokens.size());

  std::transform(tokens.begin(), tokens.end(), out.begin(),
                 [](const Token& t) { return t.toString(); });

  return out;
}

}  // namespace

// TODO: add tests for each token type

TEST(LexerTest, ScansSingleBracketAndEof) {
  Lexer lexer("[");

  const auto tokens = stringify(lexer.scanTokens());

  const std::vector<std::string> expected = {
      std::to_string(static_cast<int>(TokenType::LEFT_BRACKET)) + " [ nil",
      std::to_string(static_cast<int>(TokenType::EOF_TOKEN)) + "  nil",
  };

  EXPECT_EQ(tokens, expected);
}

TEST(LexerTest, ScansSimplePunctuationSequence) {
  Lexer lexer("[]+;");

  const auto tokens = stringify(lexer.scanTokens());

  const std::vector<std::string> expected = {
      std::to_string(static_cast<int>(TokenType::LEFT_BRACKET)) + " [ nil",
      std::to_string(static_cast<int>(TokenType::RIGHT_BRACKET)) + " ] nil",
      std::to_string(static_cast<int>(TokenType::PLUS)) + " + nil",
      std::to_string(static_cast<int>(TokenType::SEMICOLON)) + " ; nil",
      std::to_string(static_cast<int>(TokenType::EOF_TOKEN)) + "  nil",
  };

  EXPECT_EQ(tokens, expected);
}

TEST(LexerTest, ThrowsOnInvalidToken) {
  Lexer lexer("@");

  EXPECT_THROW(lexer.scanTokens(), std::invalid_argument);
}

TEST(LexerTest, ScansSingleBangAndEof) {
  Lexer lexer("!");

  const auto tokens = stringify(lexer.scanTokens());

  const std::vector<std::string> expected = {
      std::to_string(static_cast<int>(TokenType::BANG)) + " ! nil",
      std::to_string(static_cast<int>(TokenType::EOF_TOKEN)) + "  nil",
  };

  EXPECT_EQ(tokens, expected);
}

TEST(LexerTest, ScansSingleBangEqualAndEof) {
  Lexer lexer("!=");

  const auto tokens = stringify(lexer.scanTokens());

  const std::vector<std::string> expected = {
      std::to_string(static_cast<int>(TokenType::BANG_EQUAL)) + " != nil",
      std::to_string(static_cast<int>(TokenType::EOF_TOKEN)) + "  nil",
  };

  EXPECT_EQ(tokens, expected);
}

TEST(LexerTest, ScansSingleDivideAndEof) {
  Lexer lexer("/");

  const auto tokens = stringify(lexer.scanTokens());

  const std::vector<std::string> expected = {
      std::to_string(static_cast<int>(TokenType::SLASH)) + " / nil",
      std::to_string(static_cast<int>(TokenType::EOF_TOKEN)) + "  nil",
  };

  EXPECT_EQ(tokens, expected);
}

TEST(LexerTest, ScansCommentAndEof) {
  Lexer lexer("// this is a comment");

  const auto tokens = stringify(lexer.scanTokens());

  const std::vector<std::string> expected = {
      std::to_string(static_cast<int>(TokenType::EOF_TOKEN)) + "  nil",
  };

  EXPECT_EQ(tokens, expected);
}

TEST(LexerTest, ScannerIgnoresWhitespace) {
  Lexer lexer(" ");

  const auto tokens = stringify(lexer.scanTokens());

  const std::vector<std::string> expected = {
      std::to_string(static_cast<int>(TokenType::EOF_TOKEN)) + "  nil",
  };

  EXPECT_EQ(tokens, expected);
}

// TEST(LexerTest, ScansMultilineCommentAndEof) {
//   Lexer lexer(
//       "// this is a comment"
//       "// this is another comment");

//   const auto tokens = stringify(lexer.scanTokens());

//   const std::vector<std::string> expected = {
//       std::to_string(static_cast<int>(TokenType::EOF_TOKEN)) + "  nil",
//   };

//   EXPECT_EQ(tokens, expected);
// }

// TEST(LexerTest, ScansMultilineCommentAndDivideAndEof) {
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