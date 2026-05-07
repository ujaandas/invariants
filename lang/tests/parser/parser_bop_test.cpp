#include <gtest/gtest.h>

#include <variant>
#include <vector>

#include "expression.hpp"
#include "parser.hpp"
#include "token.hpp"

using namespace invariants::parser;
using TT = invariants::lexer::TokenType;
using BO = invariants::ast::BinaryOp;

namespace {
using Token = invariants::lexer::Token;

std::vector<Token> makeBinaryExpr(invariants::lexer::TokenType op) {
  return {
      Token(TT::LIT_IDENTIFIER, "a", "a", 1),
      Token(op, "", std::monostate{}, 1),
      Token(TT::LIT_IDENTIFIER, "b", "b", 1),
      Token(TT::EOF_TOKEN, "", std::monostate{}, 1),
  };
}

invariants::ast::Expr makeExpected(invariants::ast::BinaryOp op) {
  using namespace invariants::ast;

  return Expr(BinaryExpr{
      std::make_unique<Expr>(IdentifierExpr{"a"}),
      op,
      std::make_unique<Expr>(IdentifierExpr{"b"}),
  });
}
}  // namespace

struct TokenToBinaryOpCase {
  std::string name;
  invariants::lexer::TokenType token;
  invariants::ast::BinaryOp op;
};

void PrintTo(const TokenToBinaryOpCase& value, std::ostream* os) {
  *os << value.name;
}

class ParserBinaryOpTest : public testing::TestWithParam<TokenToBinaryOpCase> {
};

TEST_P(ParserBinaryOpTest, ConvertsTokenToBinaryOp) {
  const auto& param = GetParam();

  Parser parser(makeBinaryExpr(param.token));
  auto out = parser.parse();

  ASSERT_NE(out, nullptr);
  EXPECT_EQ(*out, makeExpected(param.op));
}

INSTANTIATE_TEST_SUITE_P(
    BinaryOps, ParserBinaryOpTest,
    testing::Values(
        TokenToBinaryOpCase{"Plus", TT::PLUS, BO::Add},
        TokenToBinaryOpCase{"Minus", TT::MINUS, BO::Subtract}
        // TokenToBinaryOpCase{"Star", TT::STAR, BO::Multiply}
        // TokenToBinaryOpCase{"Slash", TT::SLASH, BO::Divide},
        // TokenToBinaryOpCase{"Percentage", TT::PERCENTAGE, BO::Modulo},
        // TokenToBinaryOpCase{"LAnd", TT::LOGICAL_AND, BO::And},
        // TokenToBinaryOpCase{"LOr", TT::LOGICAL_OR, BO::Or},
        // TokenToBinaryOpCase{"EqualEqual", TT::EQUAL_EQUAL, BO::Equal},
        // TokenToBinaryOpCase{"NotEqual", TT::BANG_EQUAL, BO::NotEqual},
        // TokenToBinaryOpCase{"Greater", TT::GREATER, BO::Greater},
        // TokenToBinaryOpCase{"GreaterEqual", TT::GREATER_EQUAL,
        //                     BO::GreaterEqual},
        // TokenToBinaryOpCase{"Less", TT::LESS, BO::Less},
        // TokenToBinaryOpCase{"LessEqual", TT::LESS_EQUAL, BO::LessEqual},
        // TokenToBinaryOpCase{"MembershipIn", TT::KW_IN, BO::In},
        // TokenToBinaryOpCase{"MemberhsipNotIn", TT::KW_NOT_IN, BO::NotIn}
        ));