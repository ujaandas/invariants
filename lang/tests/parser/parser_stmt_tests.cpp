#include <gtest/gtest.h>

#include <functional>
#include <string>
#include <vector>

#include "expression.hpp"
#include "parser.hpp"
#include "statements.hpp"
#include "token.hpp"
#include "types.hpp"

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

struct StmtCase {
  std::string name;
  std::vector<Token> tokens;
  std::function<ModuleStmt()> expected;
};

void PrintTo(const StmtCase& value, std::ostream* os) { *os << value.name; }

class ParserStatementTest : public testing::TestWithParam<StmtCase> {};

TEST_P(ParserStatementTest, ParsesModuleAndSpecs) {
  const auto& param = GetParam();

  Parser parser(param.tokens);
  auto out = parser.parseModule();

  ASSERT_NE(out, nullptr);
  EXPECT_EQ(*out, param.expected());
}

INSTANTIATE_TEST_SUITE_P(
    Statements, ParserStatementTest,
    testing::Values(
        StmtCase{"empty_spec",
                 withEof({Token(TT::KW_SPEC, "spec", 1),
                          Token(TT::LIT_IDENTIFIER, "MySpec",
                                std::string("MySpec"), 1),
                          Token(TT::LEFT_BRACE, "{", 1),
                          Token(TT::RIGHT_BRACE, "}", 1)}),
                 [] {
                   std::vector<SpecPtr> specs;
                   specs.emplace_back(std::make_unique<SpecStmt>(SpecStmt{
                       std::string("MySpec"), std::vector<SpecMember>{}}));
                   return ModuleStmt{std::move(specs)};
                 }},

        StmtCase{
            "field_no_constraints",
            withEof(
                {Token(TT::KW_SPEC, "spec", 1),
                 Token(TT::LIT_IDENTIFIER, "S", std::string("S"), 1),
                 Token(TT::LEFT_BRACE, "{", 1), Token(TT::KW_FIELD, "field", 1),
                 Token(TT::LIT_IDENTIFIER, "foo", std::string("foo"), 1),
                 Token(TT::COLON, ":", 1), Token(TT::KW_NUMBER, "Number", 1),
                 Token(TT::LEFT_BRACE, "{", 1), Token(TT::RIGHT_BRACE, "}", 1),
                 Token(TT::RIGHT_BRACE, "}", 1)}),
            [] {
              std::vector<SpecMember> members;
              FieldStmt f{
                  std::string("foo"),
                  std::make_unique<Type>(SimpleType{BuiltinType::Number}),
                  std::vector<ConstraintPtr>{}};
              members.emplace_back(std::move(f));

              std::vector<SpecPtr> specs;
              specs.emplace_back(std::make_unique<SpecStmt>(
                  SpecStmt{std::string("S"), std::move(members)}));
              return ModuleStmt{std::move(specs)};
            }},

        StmtCase{
            "invariant_with_constraint",
            withEof({Token(TT::KW_SPEC, "spec", 1),
                     Token(TT::LIT_IDENTIFIER, "S", std::string("S"), 1),
                     Token(TT::LEFT_BRACE, "{", 1),
                     Token(TT::KW_INVARIANT, "invariant", 1),
                     Token(TT::LIT_IDENTIFIER, "inv", std::string("inv"), 1),
                     Token(TT::LEFT_BRACE, "{", 1),
                     Token(TT::LIT_IDENTIFIER, "x", std::string("x"), 1),
                     Token(TT::SEMICOLON, ";", 1),
                     Token(TT::RIGHT_BRACE, "}", 1),
                     Token(TT::RIGHT_BRACE, "}", 1)}),
            [] {
              std::vector<ConstraintPtr> constraints;
              constraints.emplace_back(std::make_unique<ConstraintStmt>(
                  ConstraintStmt{std::make_unique<Expr>(IdentifierExpr{"x"})}));

              InvariantStmt inv{std::string("inv"), std::move(constraints)};
              std::vector<SpecMember> members;
              members.emplace_back(std::move(inv));

              std::vector<SpecPtr> specs;
              specs.emplace_back(std::make_unique<SpecStmt>(
                  SpecStmt{std::string("S"), std::move(members)}));
              return ModuleStmt{std::move(specs)};
            }}));
