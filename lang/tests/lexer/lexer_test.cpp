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
  ;
}