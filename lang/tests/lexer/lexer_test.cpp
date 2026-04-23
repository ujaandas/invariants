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

// Test each TokenType
TEST(LexerTest, ScansTokens_LeftParen) {
  Lexer lexer("(");

  const auto tokens = lexer.scanTokens();

  EXPECT_EQ(tokens.size(), 2);
  EXPECT_EQ(tokens[0], Token(TokenType::LEFT_PAREN, "(", 1));
  EXPECT_EQ(tokens[1], Token(TokenType::EOF_TOKEN, "", 1));
}

TEST(LexerTest, ScansTokens_RightParen) {
  Lexer lexer(")");

  const auto tokens = lexer.scanTokens();

  EXPECT_EQ(tokens.size(), 2);
  EXPECT_EQ(tokens[0], Token(TokenType::RIGHT_PAREN, ")", 1));
  EXPECT_EQ(tokens[1], Token(TokenType::EOF_TOKEN, "", 1));
}

TEST(LexerTest, ScansTokens_LeftBracket) {
  Lexer lexer("[");

  const auto tokens = lexer.scanTokens();

  EXPECT_EQ(tokens.size(), 2);
  EXPECT_EQ(tokens[0], Token(TokenType::LEFT_BRACKET, "[", 1));
  EXPECT_EQ(tokens[1], Token(TokenType::EOF_TOKEN, "", 1));
}

TEST(LexerTest, ScansTokens_RightBracket) {
  Lexer lexer("]");

  const auto tokens = lexer.scanTokens();

  EXPECT_EQ(tokens.size(), 2);
  EXPECT_EQ(tokens[0], Token(TokenType::RIGHT_BRACKET, "]", 1));
  EXPECT_EQ(tokens[1], Token(TokenType::EOF_TOKEN, "", 1));
}

TEST(LexerTest, ScansTokens_LeftBrace) {
  Lexer lexer("{");

  const auto tokens = lexer.scanTokens();

  EXPECT_EQ(tokens.size(), 2);
  EXPECT_EQ(tokens[0], Token(TokenType::LEFT_BRACE, "{", 1));
  EXPECT_EQ(tokens[1], Token(TokenType::EOF_TOKEN, "", 1));
}

TEST(LexerTest, ScansTokens_RightBrace) {
  Lexer lexer("}");

  const auto tokens = lexer.scanTokens();

  EXPECT_EQ(tokens.size(), 2);
  EXPECT_EQ(tokens[0], Token(TokenType::RIGHT_BRACE, "}", 1));
  EXPECT_EQ(tokens[1], Token(TokenType::EOF_TOKEN, "", 1));
}

TEST(LexerTest, ScansTokens_Colon) {
  Lexer lexer(":");

  const auto tokens = lexer.scanTokens();

  EXPECT_EQ(tokens.size(), 2);
  EXPECT_EQ(tokens[0], Token(TokenType::COLON, ":", 1));
  EXPECT_EQ(tokens[1], Token(TokenType::EOF_TOKEN, "", 1));
}

TEST(LexerTest, ScansTokens_Comma) {
  Lexer lexer(",");

  const auto tokens = lexer.scanTokens();

  EXPECT_EQ(tokens.size(), 2);
  EXPECT_EQ(tokens[0], Token(TokenType::COMMA, ",", 1));
  EXPECT_EQ(tokens[1], Token(TokenType::EOF_TOKEN, "", 1));
}

TEST(LexerTest, ScansTokens_Dot) {
  Lexer lexer(".");

  const auto tokens = lexer.scanTokens();

  EXPECT_EQ(tokens.size(), 2);
  EXPECT_EQ(tokens[0], Token(TokenType::DOT, ".", 1));
  EXPECT_EQ(tokens[1], Token(TokenType::EOF_TOKEN, "", 1));
}

TEST(LexerTest, ScansTokens_Semicolon) {
  Lexer lexer(";");

  const auto tokens = lexer.scanTokens();

  EXPECT_EQ(tokens.size(), 2);
  EXPECT_EQ(tokens[0], Token(TokenType::SEMICOLON, ";", 1));
  EXPECT_EQ(tokens[1], Token(TokenType::EOF_TOKEN, "", 1));
}

TEST(LexerTest, ScansTokens_Plus) {
  Lexer lexer("+");

  const auto tokens = lexer.scanTokens();

  EXPECT_EQ(tokens.size(), 2);
  EXPECT_EQ(tokens[0], Token(TokenType::PLUS, "+", 1));
  EXPECT_EQ(tokens[1], Token(TokenType::EOF_TOKEN, "", 1));
}

TEST(LexerTest, ScansTokens_Slash) {
  Lexer lexer("/");

  const auto tokens = lexer.scanTokens();

  EXPECT_EQ(tokens.size(), 2);
  EXPECT_EQ(tokens[0], Token(TokenType::SLASH, "/", 1));
  EXPECT_EQ(tokens[1], Token(TokenType::EOF_TOKEN, "", 1));
}

TEST(LexerTest, ScansTokens_Star) {
  Lexer lexer("*");

  const auto tokens = lexer.scanTokens();

  EXPECT_EQ(tokens.size(), 2);
  EXPECT_EQ(tokens[0], Token(TokenType::STAR, "*", 1));
  EXPECT_EQ(tokens[1], Token(TokenType::EOF_TOKEN, "", 1));
}

TEST(LexerTest, ScansTokens_Percentage) {
  Lexer lexer("%");

  const auto tokens = lexer.scanTokens();

  EXPECT_EQ(tokens.size(), 2);
  EXPECT_EQ(tokens[0], Token(TokenType::PERCENTAGE, "%", 1));
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

TEST(LexerTest, ScansSingleEqual) {
  Lexer lexer("=");

  const auto tokens = lexer.scanTokens();

  EXPECT_EQ(tokens.size(), 2);
  EXPECT_EQ(tokens[0], Token(TokenType::EQUAL, "=", 1));
  EXPECT_EQ(tokens[1], Token(TokenType::EOF_TOKEN, "", 1));
}

TEST(LexerTest, ScansEqualEqual) {
  Lexer lexer("==");

  const auto tokens = lexer.scanTokens();

  EXPECT_EQ(tokens.size(), 2);
  EXPECT_EQ(tokens[0], Token(TokenType::EQUAL_EQUAL, "==", 1));
  EXPECT_EQ(tokens[1], Token(TokenType::EOF_TOKEN, "", 1));
}

TEST(LexerTest, ScansSingleGreater) {
  Lexer lexer(">");

  const auto tokens = lexer.scanTokens();

  EXPECT_EQ(tokens.size(), 2);
  EXPECT_EQ(tokens[0], Token(TokenType::GREATER, ">", 1));
  EXPECT_EQ(tokens[1], Token(TokenType::EOF_TOKEN, "", 1));
}

TEST(LexerTest, ScansGreaterEqual) {
  Lexer lexer(">=");

  const auto tokens = lexer.scanTokens();

  EXPECT_EQ(tokens.size(), 2);
  EXPECT_EQ(tokens[0], Token(TokenType::GREATER_EQUAL, ">=", 1));
  EXPECT_EQ(tokens[1], Token(TokenType::EOF_TOKEN, "", 1));
}

TEST(LexerTest, ScansSingleLess) {
  Lexer lexer("<");

  const auto tokens = lexer.scanTokens();

  EXPECT_EQ(tokens.size(), 2);
  EXPECT_EQ(tokens[0], Token(TokenType::LESS, "<", 1));
  EXPECT_EQ(tokens[1], Token(TokenType::EOF_TOKEN, "", 1));
}

TEST(LexerTest, ScansLessEqual) {
  Lexer lexer("<=");

  const auto tokens = lexer.scanTokens();

  EXPECT_EQ(tokens.size(), 2);
  EXPECT_EQ(tokens[0], Token(TokenType::LESS_EQUAL, "<=", 1));
  EXPECT_EQ(tokens[1], Token(TokenType::EOF_TOKEN, "", 1));
}

TEST(LexerTest, ScansSingleMinus) {
  Lexer lexer("-");

  const auto tokens = lexer.scanTokens();

  EXPECT_EQ(tokens.size(), 2);
  EXPECT_EQ(tokens[0], Token(TokenType::MINUS, "-", 1));
  EXPECT_EQ(tokens[1], Token(TokenType::EOF_TOKEN, "", 1));
}

TEST(LexerTest, ScansArrow) {
  Lexer lexer("->");

  const auto tokens = lexer.scanTokens();

  EXPECT_EQ(tokens.size(), 2);
  EXPECT_EQ(tokens[0], Token(TokenType::ARROW, "->", 1));
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

TEST(LexerTest, ScansIdentifierPrependUnderscore) {
  Lexer lexer("_foobar");
  EXPECT_THROW(lexer.scanTokens(), std::invalid_argument);
}

TEST(LexerTest, ScansIdentifierAppendUnderscore) {
  Lexer lexer("foobar_");

  const auto tokens = lexer.scanTokens();

  EXPECT_EQ(tokens.size(), 2);
  EXPECT_EQ(tokens[0], Token(TokenType::LIT_IDENTIFIER, "foobar_", 1));
  EXPECT_EQ(tokens[1], Token(TokenType::EOF_TOKEN, "", 1));
}

TEST(LexerTest, ScansIdentifierMiddleUnderscore) {
  Lexer lexer("foo_bar");

  const auto tokens = lexer.scanTokens();

  EXPECT_EQ(tokens.size(), 2);
  EXPECT_EQ(tokens[0], Token(TokenType::LIT_IDENTIFIER, "foo_bar", 1));
  EXPECT_EQ(tokens[1], Token(TokenType::EOF_TOKEN, "", 1));
}

TEST(LexerTest, ScansSpecKeyword) {
  Lexer lexer("spec");

  const auto tokens = lexer.scanTokens();

  EXPECT_EQ(tokens.size(), 2);
  EXPECT_EQ(tokens[0], Token(TokenType::KW_SPEC, "spec", 1));
  EXPECT_EQ(tokens[1], Token(TokenType::EOF_TOKEN, "", 1));
}

TEST(LexerTest, ScansFieldKeyword) {
  Lexer lexer("field");

  const auto tokens = lexer.scanTokens();

  EXPECT_EQ(tokens.size(), 2);
  EXPECT_EQ(tokens[0], Token(TokenType::KW_FIELD, "field", 1));
  EXPECT_EQ(tokens[1], Token(TokenType::EOF_TOKEN, "", 1));
}

TEST(LexerTest, ScansCheckKeyword) {
  Lexer lexer("check");

  const auto tokens = lexer.scanTokens();

  EXPECT_EQ(tokens.size(), 2);
  EXPECT_EQ(tokens[0], Token(TokenType::KW_CHECK, "check", 1));
  EXPECT_EQ(tokens[1], Token(TokenType::EOF_TOKEN, "", 1));
}

TEST(LexerTest, ScansInvariantKeyword) {
  Lexer lexer("invariant");

  const auto tokens = lexer.scanTokens();

  EXPECT_EQ(tokens.size(), 2);
  EXPECT_EQ(tokens[0], Token(TokenType::KW_INVARIANT, "invariant", 1));
  EXPECT_EQ(tokens[1], Token(TokenType::EOF_TOKEN, "", 1));
}

TEST(LexerTest, ScansBooleanKeyword) {
  Lexer lexer("Boolean");

  const auto tokens = lexer.scanTokens();

  EXPECT_EQ(tokens.size(), 2);
  EXPECT_EQ(tokens[0], Token(TokenType::KW_BOOLEAN, "Boolean", 1));
  EXPECT_EQ(tokens[1], Token(TokenType::EOF_TOKEN, "", 1));
}

TEST(LexerTest, ScansArrayKeyword) {
  Lexer lexer("Array");

  const auto tokens = lexer.scanTokens();

  EXPECT_EQ(tokens.size(), 2);
  EXPECT_EQ(tokens[0], Token(TokenType::KW_ARRAY, "Array", 1));
  EXPECT_EQ(tokens[1], Token(TokenType::EOF_TOKEN, "", 1));
}

TEST(LexerTest, ScansNullKeyword) {
  Lexer lexer("Null");

  const auto tokens = lexer.scanTokens();

  EXPECT_EQ(tokens.size(), 2);
  EXPECT_EQ(tokens[0], Token(TokenType::KW_NULL, "Null", 1));
  EXPECT_EQ(tokens[1], Token(TokenType::EOF_TOKEN, "", 1));
}

TEST(LexerTest, ScansStringKeyword) {
  Lexer lexer("String");

  const auto tokens = lexer.scanTokens();

  EXPECT_EQ(tokens.size(), 2);
  EXPECT_EQ(tokens[0], Token(TokenType::KW_STRING, "String", 1));
  EXPECT_EQ(tokens[1], Token(TokenType::EOF_TOKEN, "", 1));
}

TEST(LexerTest, ScansNumberKeyword) {
  Lexer lexer("Number");

  const auto tokens = lexer.scanTokens();

  EXPECT_EQ(tokens.size(), 2);
  EXPECT_EQ(tokens[0], Token(TokenType::KW_NUMBER, "Number", 1));
  EXPECT_EQ(tokens[1], Token(TokenType::EOF_TOKEN, "", 1));
}

TEST(LexerTest, ScansIntegerKeyword) {
  Lexer lexer("Integer");

  const auto tokens = lexer.scanTokens();

  EXPECT_EQ(tokens.size(), 2);
  EXPECT_EQ(tokens[0], Token(TokenType::KW_INTEGER, "Integer", 1));
  EXPECT_EQ(tokens[1], Token(TokenType::EOF_TOKEN, "", 1));
}

TEST(LexerTest, ScansInKeyword) {
  Lexer lexer("IN");

  const auto tokens = lexer.scanTokens();

  EXPECT_EQ(tokens.size(), 2);
  EXPECT_EQ(tokens[0], Token(TokenType::KW_IN, "IN", 1));
  EXPECT_EQ(tokens[1], Token(TokenType::EOF_TOKEN, "", 1));
}

TEST(LexerTest, ScansNotInKeyword) {
  Lexer lexer("NIN");

  const auto tokens = lexer.scanTokens();

  EXPECT_EQ(tokens.size(), 2);
  EXPECT_EQ(tokens[0], Token(TokenType::KW_NOT_IN, "NIN", 1));
  EXPECT_EQ(tokens[1], Token(TokenType::EOF_TOKEN, "", 1));
}

TEST(LexerTest, ScansContainsKeyword) {
  Lexer lexer("NI");

  const auto tokens = lexer.scanTokens();

  EXPECT_EQ(tokens.size(), 2);
  EXPECT_EQ(tokens[0], Token(TokenType::KW_CONTAINS, "NI", 1));
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

TEST(LexerTest, ScansLiteralBooleanTrue) {
  Lexer lexer("true");

  const auto tokens = lexer.scanTokens();

  EXPECT_EQ(tokens.size(), 2);
  EXPECT_EQ(tokens[0], Token(TokenType::LIT_BOOLEAN_T, "true", true, 1));
  EXPECT_EQ(tokens[1], Token(TokenType::EOF_TOKEN, "", 1));
}

TEST(LexerTest, ScansLiteralBooleanFalse) {
  Lexer lexer("false");

  const auto tokens = lexer.scanTokens();

  EXPECT_EQ(tokens.size(), 2);
  EXPECT_EQ(tokens[0], Token(TokenType::LIT_BOOLEAN_F, "false", false, 1));
  EXPECT_EQ(tokens[1], Token(TokenType::EOF_TOKEN, "", 1));
}

TEST(LexerTest, ScansLiteralNull) {
  Lexer lexer("null");

  const auto tokens = lexer.scanTokens();

  EXPECT_EQ(tokens.size(), 2);
  EXPECT_EQ(tokens[0], Token(TokenType::LIT_NULL, "null", 1));
  EXPECT_EQ(tokens[1], Token(TokenType::EOF_TOKEN, "", 1));
}