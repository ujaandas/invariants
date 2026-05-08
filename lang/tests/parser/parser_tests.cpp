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

TEST_P(ParserSingleExprTest, ParsesSingleValueExpressions) {
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
