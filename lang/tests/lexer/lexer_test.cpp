#include "lexer.hpp"

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <vector>

#include "token.hpp"

using invariants::lexer::Lexer;
using invariants::lexer::Token;
using invariants::lexer::TokenType;

namespace {

struct TokenCase {
  std::string input;
  std::vector<Token> expected;
  std::string name;
};

void PrintTo(const TokenCase& tc, std::ostream* os) { *os << tc.name; }

}  // namespace

class LexerScansTokenTest : public testing::TestWithParam<TokenCase> {};

TEST_P(LexerScansTokenTest, ScansToken) {
  const auto& param = GetParam();

  Lexer lexer(param.input);
  auto tokens = lexer.scanTokens();

  EXPECT_EQ(tokens, param.expected);
}

INSTANTIATE_TEST_SUITE_P(
    SingleChar, LexerScansTokenTest,
    testing::Values(TokenCase{"(",
                              {Token{TokenType::LEFT_PAREN, "(", 1},
                               Token{TokenType::EOF_TOKEN, "", 1}},
                              "LeftParen"},
                    TokenCase{")",
                              {Token{TokenType::RIGHT_PAREN, ")", 1},
                               Token{TokenType::EOF_TOKEN, "", 1}},
                              "RightParen"},
                    TokenCase{"[",
                              {Token{TokenType::LEFT_BRACKET, "[", 1},
                               Token{TokenType::EOF_TOKEN, "", 1}},
                              "LeftBracket"},
                    TokenCase{"]",
                              {Token{TokenType::RIGHT_BRACKET, "]", 1},
                               Token{TokenType::EOF_TOKEN, "", 1}},
                              "RightBracket"},
                    TokenCase{"{",
                              {Token{TokenType::LEFT_BRACE, "{", 1},
                               Token{TokenType::EOF_TOKEN, "", 1}},
                              "LeftBrace"},
                    TokenCase{"}",
                              {Token{TokenType::RIGHT_BRACE, "}", 1},
                               Token{TokenType::EOF_TOKEN, "", 1}},
                              "RightBrace"},
                    TokenCase{":",
                              {Token{TokenType::COLON, ":", 1},
                               Token{TokenType::EOF_TOKEN, "", 1}},
                              "Colon"},
                    TokenCase{",",
                              {Token{TokenType::COMMA, ",", 1},
                               Token{TokenType::EOF_TOKEN, "", 1}},
                              "Comma"},
                    TokenCase{".",
                              {Token{TokenType::DOT, ".", 1},
                               Token{TokenType::EOF_TOKEN, "", 1}},
                              "Dot"},
                    TokenCase{";",
                              {Token{TokenType::SEMICOLON, ";", 1},
                               Token{TokenType::EOF_TOKEN, "", 1}},
                              "Semicolon"},
                    TokenCase{"+",
                              {Token{TokenType::PLUS, "+", 1},
                               Token{TokenType::EOF_TOKEN, "", 1}},
                              "Plus"},
                    TokenCase{"-",
                              {Token{TokenType::MINUS, "-", 1},
                               Token{TokenType::EOF_TOKEN, "", 1}},
                              "Minus"},
                    TokenCase{"/",
                              {Token{TokenType::SLASH, "/", 1},
                               Token{TokenType::EOF_TOKEN, "", 1}},
                              "Slash"},
                    TokenCase{"*",
                              {Token{TokenType::STAR, "*", 1},
                               Token{TokenType::EOF_TOKEN, "", 1}},
                              "Star"},
                    TokenCase{"%",
                              {Token{TokenType::PERCENTAGE, "%", 1},
                               Token{TokenType::EOF_TOKEN, "", 1}},
                              "Percentage"},
                    TokenCase{"!",
                              {Token{TokenType::BANG, "!", 1},
                               Token{TokenType::EOF_TOKEN, "", 1}},
                              "Bang"},
                    TokenCase{"=",
                              {Token{TokenType::EQUAL, "=", 1},
                               Token{TokenType::EOF_TOKEN, "", 1}},
                              "Equal"},
                    TokenCase{">",
                              {Token{TokenType::GREATER, ">", 1},
                               Token{TokenType::EOF_TOKEN, "", 1}},
                              "Greater"},
                    TokenCase{"<",
                              {Token{TokenType::LESS, "<", 1},
                               Token{TokenType::EOF_TOKEN, "", 1}},
                              "Less"}));

INSTANTIATE_TEST_SUITE_P(
    MultiChar, LexerScansTokenTest,
    testing::Values(TokenCase{"!=",
                              {Token{TokenType::BANG_EQUAL, "!=", 1},
                               Token{TokenType::EOF_TOKEN, "", 1}},
                              "BangEquals"},
                    TokenCase{"==",
                              {Token{TokenType::EQUAL_EQUAL, "==", 1},
                               Token{TokenType::EOF_TOKEN, "", 1}},
                              "EqualEquals"},
                    TokenCase{">=",
                              {Token{TokenType::GREATER_EQUAL, ">=", 1},
                               Token{TokenType::EOF_TOKEN, "", 1}},
                              "GreaterEquals"},
                    TokenCase{"<=",
                              {Token{TokenType::LESS_EQUAL, "<=", 1},
                               Token{TokenType::EOF_TOKEN, "", 1}},
                              "LessEquals"},
                    TokenCase{"->",
                              {Token{TokenType::ARROW, "->", 1},
                               Token{TokenType::EOF_TOKEN, "", 1}},
                              "Arrow"}));

INSTANTIATE_TEST_SUITE_P(
    StructuralKeywords, LexerScansTokenTest,
    testing::Values(TokenCase{"spec",
                              {Token{TokenType::KW_SPEC, "spec", 1},
                               Token{TokenType::EOF_TOKEN, "", 1}},
                              "Spec"},
                    TokenCase{"field",
                              {Token{TokenType::KW_FIELD, "field", 1},
                               Token{TokenType::EOF_TOKEN, "", 1}},
                              "Field"},
                    TokenCase{"check",
                              {Token{TokenType::KW_CHECK, "check", 1},
                               Token{TokenType::EOF_TOKEN, "", 1}},
                              "Check"},
                    TokenCase{"invariant",
                              {Token{TokenType::KW_INVARIANT, "invariant", 1},
                               Token{TokenType::EOF_TOKEN, "", 1}},
                              "Invariant"}));

INSTANTIATE_TEST_SUITE_P(
    TypeKeywords, LexerScansTokenTest,
    testing::Values(TokenCase{"Boolean",
                              {Token{TokenType::KW_BOOLEAN, "Boolean", 1},
                               Token{TokenType::EOF_TOKEN, "", 1}},
                              "Boolean"},
                    TokenCase{"Array",
                              {Token{TokenType::KW_ARRAY, "Array", 1},
                               Token{TokenType::EOF_TOKEN, "", 1}},
                              "Array"},
                    TokenCase{"Null",
                              {Token{TokenType::KW_NULL, "Null", 1},
                               Token{TokenType::EOF_TOKEN, "", 1}},
                              "Null"},
                    TokenCase{"String",
                              {Token{TokenType::KW_STRING, "String", 1},
                               Token{TokenType::EOF_TOKEN, "", 1}},
                              "String"},
                    TokenCase{"Number",
                              {Token{TokenType::KW_NUMBER, "Number", 1},
                               Token{TokenType::EOF_TOKEN, "", 1}},
                              "Number"},
                    TokenCase{"Integer",
                              {Token{TokenType::KW_INTEGER, "Integer", 1},
                               Token{TokenType::EOF_TOKEN, "", 1}},
                              "Integer"}));

INSTANTIATE_TEST_SUITE_P(
    Operators, LexerScansTokenTest,
    testing::Values(TokenCase{"IN",
                              {Token{TokenType::KW_IN, "IN", 1},
                               Token{TokenType::EOF_TOKEN, "", 1}},
                              "In"},
                    TokenCase{"NIN",
                              {Token{TokenType::KW_NOT_IN, "NIN", 1},
                               Token{TokenType::EOF_TOKEN, "", 1}},
                              "Not in"},
                    TokenCase{"NI",
                              {Token{TokenType::KW_CONTAINS, "NI", 1},
                               Token{TokenType::EOF_TOKEN, "", 1}},
                              "Contains"}));

TEST(LexerTest, IgnoresWhitespaceAndTracksNewlineForEof) {
  Lexer lexer(" \t\r\n");

  const std::vector<Token> expected = {
      Token{TokenType::EOF_TOKEN, "", 2},
  };

  EXPECT_EQ(lexer.scanTokens(), expected);
}

TEST(LexerTest, SkipsSingleLineComment) {
  Lexer lexer("// comment only");

  const std::vector<Token> expected = {
      Token{TokenType::EOF_TOKEN, "", 1},
  };

  EXPECT_EQ(lexer.scanTokens(), expected);
}

TEST(LexerTest, ScansSlashAfterCommentLines) {
  Lexer lexer("// first\n// second\n/");

  const std::vector<Token> expected = {
      Token{TokenType::SLASH, "/", 3},
      Token{TokenType::EOF_TOKEN, "", 3},
  };

  EXPECT_EQ(lexer.scanTokens(), expected);
}

TEST(LexerTest, ScansStringLiteralWithLiteralValue) {
  Lexer lexer("\"Hello, world!\"");

  const std::vector<Token> expected = {
      Token{TokenType::LIT_STRING, "\"Hello, world!\"",
            std::string("Hello, world!"), 1},
      Token{TokenType::EOF_TOKEN, "", 1},
  };

  EXPECT_EQ(lexer.scanTokens(), expected);
}

TEST(LexerTest, ThrowsOnUnterminatedString) {
  Lexer lexer("\"unterminated");

  EXPECT_THROW(lexer.scanTokens(), std::range_error);
}

TEST(LexerTest, ScansIntegerAndNumberLiterals) {
  Lexer lexer("123 45.5");

  const std::vector<Token> expected = {
      Token{TokenType::LIT_INTEGER, "123", 123, 1},
      Token{TokenType::LIT_NUMBER, "45.5", 45.5, 1},
      Token{TokenType::EOF_TOKEN, "", 1},
  };

  EXPECT_EQ(lexer.scanTokens(), expected);
}

TEST(LexerTest, TreatsLeadingDecimalAsDotThenInteger) {
  Lexer lexer(".456");

  const std::vector<Token> expected = {
      Token{TokenType::DOT, ".", 1},
      Token{TokenType::LIT_INTEGER, "456", 456, 1},
      Token{TokenType::EOF_TOKEN, "", 1},
  };

  EXPECT_EQ(lexer.scanTokens(), expected);
}

TEST(LexerTest, ScansIdentifiersWithUnderscoresAfterFirstChar) {
  Lexer lexer("foo_bar bar_2");

  const std::vector<Token> expected = {
      Token{TokenType::LIT_IDENTIFIER, "foo_bar", 1},
      Token{TokenType::LIT_IDENTIFIER, "bar_2", 1},
      Token{TokenType::EOF_TOKEN, "", 1},
  };

  EXPECT_EQ(lexer.scanTokens(), expected);
}

TEST(LexerTest, RejectsLeadingUnderscoreIdentifier) {
  Lexer lexer("_bad");

  EXPECT_THROW(lexer.scanTokens(), std::invalid_argument);
}

TEST(LexerTest, DifferentiatesKeywordAndIdentifierPrefixes) {
  Lexer lexer("spec specX Boolean true false null");

  const std::vector<Token> expected = {
      Token{TokenType::KW_SPEC, "spec", 1},
      Token{TokenType::LIT_IDENTIFIER, "specX", 1},
      Token{TokenType::KW_BOOLEAN, "Boolean", 1},
      Token{TokenType::LIT_BOOLEAN_T, "true", true, 1},
      Token{TokenType::LIT_BOOLEAN_F, "false", false, 1},
      Token{TokenType::LIT_NULL, "null", 1},
      Token{TokenType::EOF_TOKEN, "", 1},
  };

  EXPECT_EQ(lexer.scanTokens(), expected);
}

TEST(LexerTest, ThrowsOnInvalidToken) {
  Lexer lexer("@");

  EXPECT_THROW(lexer.scanTokens(), std::invalid_argument);
}

TEST(LexerTest, ScansMixedProgramAndTracksTokenLines) {
  Lexer lexer(
      "spec User {\n"
      "field age: Integer\n"
      "check age >= 18\n"
      "}");

  const std::vector<Token> expected = {
      Token{TokenType::KW_SPEC, "spec", 1},
      Token{TokenType::LIT_IDENTIFIER, "User", 1},
      Token{TokenType::LEFT_BRACE, "{", 1},
      Token{TokenType::KW_FIELD, "field", 2},
      Token{TokenType::LIT_IDENTIFIER, "age", 2},
      Token{TokenType::COLON, ":", 2},
      Token{TokenType::KW_INTEGER, "Integer", 2},
      Token{TokenType::KW_CHECK, "check", 3},
      Token{TokenType::LIT_IDENTIFIER, "age", 3},
      Token{TokenType::GREATER_EQUAL, ">=", 3},
      Token{TokenType::LIT_INTEGER, "18", 18, 3},
      Token{TokenType::RIGHT_BRACE, "}", 4},
      Token{TokenType::EOF_TOKEN, "", 4},
  };

  EXPECT_EQ(lexer.scanTokens(), expected);
}

TEST(LexerTest, RepeatedScanTokensCallsReturnSameResult) {
  Lexer lexer("check age >= 18");

  const std::vector<Token> expected = {
      Token{TokenType::KW_CHECK, "check", 1},
      Token{TokenType::LIT_IDENTIFIER, "age", 1},
      Token{TokenType::GREATER_EQUAL, ">=", 1},
      Token{TokenType::LIT_INTEGER, "18", 18, 1},
      Token{TokenType::EOF_TOKEN, "", 1},
  };

  EXPECT_EQ(lexer.scanTokens(), expected);
  EXPECT_EQ(lexer.scanTokens(), expected);
}