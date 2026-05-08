#include <gtest/gtest.h>

#include <functional>
#include <string>
#include <vector>

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

struct TypeCase {
  std::string name;
  std::vector<Token> tokens;
  std::function<Type()> expected;
};

void PrintTo(const TypeCase& value, std::ostream* os) { *os << value.name; }

class ParserTypeTest : public testing::TestWithParam<TypeCase> {};

TEST_P(ParserTypeTest, ParsesTypes) {
  const auto& param = GetParam();

  Parser parser(param.tokens);
  auto out = parser.parseType();

  ASSERT_NE(out, nullptr);
  EXPECT_EQ(*out, param.expected());
}

INSTANTIATE_TEST_SUITE_P(
    Types, ParserTypeTest,
    testing::Values(
        TypeCase{"builtin_number", withEof({Token(TT::KW_NUMBER, "Number", 1)}),
                 [] { return Type(SimpleType{BuiltinType::Number}); }},
        TypeCase{"builtin_integer",
                 withEof({Token(TT::KW_INTEGER, "Integer", 1)}),
                 [] { return Type(SimpleType{BuiltinType::Integer}); }},
        TypeCase{"builtin_string", withEof({Token(TT::KW_STRING, "String", 1)}),
                 [] { return Type(SimpleType{BuiltinType::String}); }},
        TypeCase{"builtin_boolean",
                 withEof({Token(TT::KW_BOOLEAN, "Boolean", 1)}),
                 [] { return Type(SimpleType{BuiltinType::Boolean}); }},
        TypeCase{
            "named_type",
            withEof({Token(TT::LIT_IDENTIFIER, "Foo", std::string("Foo"), 1)}),
            [] { return Type(SimpleType{std::string("Foo")}); }},
        TypeCase{"parenthesized",
                 withEof({Token(TT::LEFT_PAREN, "(", 1),
                          Token(TT::KW_NUMBER, "Number", 1),
                          Token(TT::RIGHT_PAREN, ")", 1)}),
                 [] { return Type(SimpleType{BuiltinType::Number}); }},
        TypeCase{
            "array_of_number",
            withEof({Token(TT::KW_ARRAY, "Array", 1), Token(TT::LESS, "<", 1),
                     Token(TT::KW_NUMBER, "Number", 1),
                     Token(TT::GREATER, ">", 1)}),
            [] {
              return Type(ArrayType{
                  std::make_unique<Type>(SimpleType{BuiltinType::Number})});
            }},
        TypeCase{
            "map_string_number",
            withEof({Token(TT::KW_MAP, "Map", 1), Token(TT::LESS, "<", 1),
                     Token(TT::KW_STRING, "String", 1),
                     Token(TT::COMMA, ",", 1),
                     Token(TT::KW_NUMBER, "Number", 1),
                     Token(TT::GREATER, ">", 1)}),
            [] {
              return Type(MapType{
                  std::make_unique<Type>(SimpleType{BuiltinType::String}),
                  std::make_unique<Type>(SimpleType{BuiltinType::Number})});
            }},
        TypeCase{
            "nested_array_map",
            withEof({Token(TT::KW_ARRAY, "Array", 1), Token(TT::LESS, "<", 1),
                     Token(TT::KW_MAP, "Map", 1), Token(TT::LESS, "<", 1),
                     Token(TT::KW_STRING, "String", 1),
                     Token(TT::COMMA, ",", 1),
                     Token(TT::KW_NUMBER, "Number", 1),
                     Token(TT::GREATER, ">", 1), Token(TT::GREATER, ">", 1)}),
            [] {
              return Type(ArrayType{std::make_unique<Type>(MapType{
                  std::make_unique<Type>(SimpleType{BuiltinType::String}),
                  std::make_unique<Type>(SimpleType{BuiltinType::Number})})});
            }},
        TypeCase{"map_of_id",
                 withEof({Token(TT::KW_MAP, "Map", 1), Token(TT::LESS, "<", 1),
                          Token(TT::LIT_IDENTIFIER, "K", std::string("K"), 1),
                          Token(TT::COMMA, ",", 1),
                          Token(TT::LIT_IDENTIFIER, "V", std::string("V"), 1),
                          Token(TT::GREATER, ">", 1)}),
                 [] {
                   return Type(MapType{
                       std::make_unique<Type>(SimpleType{std::string("K")}),
                       std::make_unique<Type>(SimpleType{std::string("V")})});
                 }}));
