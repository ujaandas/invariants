#include <gtest/gtest.h>

#include <functional>
#include <string>
#include <vector>

#include "expression.hpp"
#include "parser.hpp"
#include "token.hpp"

using namespace invariants::ast;
using namespace invariants::parser;
using TT = invariants::lexer::TokenType;

namespace {

using Token = invariants::lexer::Token;

std::vector<Token> withEof(std::vector<Token> tokens) {
  tokens.emplace_back(TT::EOF_TOKEN, "", std::monostate{}, 1);
  return tokens;
}

}  // namespace

struct ExprCase {
  std::string name;
  std::vector<Token> tokens;
  std::function<Expr()> expected;
};

void PrintTo(const ExprCase& value, std::ostream* os) { *os << value.name; }

class ParserSingleExprTest : public testing::TestWithParam<ExprCase> {};

class ParserWrappedExprTest : public testing::TestWithParam<ExprCase> {};

TEST_P(ParserSingleExprTest, ParsesSingleValueExpressions) {
  const auto& param = GetParam();

  Parser parser(param.tokens);
  auto out = parser.parseExpr();

  ASSERT_NE(out, nullptr);
  EXPECT_EQ(*out, param.expected());
}

TEST_P(ParserWrappedExprTest, ParsesSimpleWrappedExpressions) {
  const auto& param = GetParam();

  Parser parser(param.tokens);
  auto out = parser.parseExpr();

  ASSERT_NE(out, nullptr);
  EXPECT_EQ(*out, param.expected());
}

INSTANTIATE_TEST_SUITE_P(
    PrimaryValues, ParserSingleExprTest,
    testing::Values(
        ExprCase{"number_literal",
                 withEof({Token(TT::LIT_NUMBER, "3.5", 3.5, 1)}),
                 [] { return Expr(LiteralExpr{3.5}); }},
        ExprCase{"integer_literal",
                 withEof({Token(TT::LIT_INTEGER, "7", 7, 1)}),
                 [] { return Expr(LiteralExpr{7.0}); }},
        ExprCase{"string_literal",
                 withEof({Token(TT::LIT_STRING, "\"hello\"",
                                std::string("hello"), 1)}),
                 [] { return Expr(LiteralExpr{std::string("hello")}); }},
        ExprCase{"boolean_true",
                 withEof({Token(TT::LIT_BOOLEAN_T, "true", true, 1)}),
                 [] { return Expr(LiteralExpr{true}); }},
        ExprCase{"boolean_false",
                 withEof({Token(TT::LIT_BOOLEAN_F, "false", false, 1)}),
                 [] { return Expr(LiteralExpr{false}); }},
        ExprCase{"null_literal",
                 withEof({Token(TT::LIT_NULL, "null", std::monostate{}, 1)}),
                 [] { return Expr(LiteralExpr{nullptr}); }},
        ExprCase{"identifier",
                 withEof({Token(TT::LIT_IDENTIFIER, "value", "value", 1)}),
                 [] { return Expr(IdentifierExpr{"value"}); }},
        ExprCase{"this_keyword", withEof({Token(TT::KW_THIS, "this", 1)}),
                 [] { return Expr(ThisExpr{}); }}));

INSTANTIATE_TEST_SUITE_P(
    SimpleWrappers, ParserWrappedExprTest,
    testing::Values(ExprCase{"grouped_identifier",
                             withEof({Token(TT::LEFT_PAREN, "(", 1),
                                      Token(TT::LIT_IDENTIFIER, "x", "x", 1),
                                      Token(TT::RIGHT_PAREN, ")", 1)}),
                             [] {
                               return Expr(GroupingExpr{std::make_unique<Expr>(
                                   IdentifierExpr{"x"})});
                             }},
                    ExprCase{"single_item_list",
                             withEof({Token(TT::LEFT_BRACKET, "[", 1),
                                      Token(TT::LIT_IDENTIFIER, "x", "x", 1),
                                      Token(TT::RIGHT_BRACKET, "]", 1)}),
                             [] {
                               std::vector<ExprPtr> elements;
                               elements.emplace_back(
                                   std::make_unique<Expr>(IdentifierExpr{"x"}));
                               return Expr(ListExpr{std::move(elements)});
                             }},
                    ExprCase{
                        "unary_not",
                        withEof({Token(TT::BANG, "!", 1),
                                 Token(TT::LIT_BOOLEAN_T, "true", true, 1)}),
                        [] {
                          return Expr(UnaryExpr{
                              UnaryOp::Not,
                              std::make_unique<Expr>(LiteralExpr{true})});
                        }},
                    ExprCase{"unary_negate",
                             withEof({Token(TT::MINUS, "-", 1),
                                      Token(TT::LIT_INTEGER, "9", 9, 1)}),
                             [] {
                               return Expr(UnaryExpr{
                                   UnaryOp::Negate,
                                   std::make_unique<Expr>(LiteralExpr{9.0})});
                             }}));
