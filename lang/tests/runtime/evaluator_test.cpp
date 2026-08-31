#include "evaluator.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "binder.hpp"
#include "bound_expr.hpp"
#include "lexer.hpp"
#include "parser.hpp"

using namespace invariants::lexer;
using namespace invariants::parser;
using namespace invariants::binder;
using namespace invariants::runtime;

namespace {

invariants::ast::ModulePtr parseSource(const std::string& source) {
  Lexer lexer(source);
  auto tokens = lexer.scanTokens();

  Parser parser(tokens);
  auto module = parser.parseModule();

  EXPECT_NE(module, nullptr);
  return module;
}

}  // namespace

TEST(EvaluatorTest, EvaluatesArithmeticAndUpcasting) {
  std::string source = R"(
    spec Test {
      field quantity: Integer {}
      field price: Number {}
      field result: Number {
        // quantity is an Integer (10), price is a Number (2.5). Result should be 25.0
        value == this.quantity * this.price; 
      }
    }
  )";
  auto ast = parseSource(source);
  Binder binder;
  BoundModule bound = binder.bind(*ast);  // Binder is now kept alive!

  // fields[2] is result
  const auto& constraintExpr = bound.specs[0].fields[2].constraints[0].expr;

  // Extract the math part (the right side of `value == ...`)
  const auto& equalityExpr = std::get<BoundBinaryExpr>(constraintExpr->value);
  const auto& mathExpr = equalityExpr.right;

  Environment env = {{"quantity", 10}, {"price", 2.5}};

  Evaluator eval;
  Value res = eval.evaluate(*mathExpr, env);

  // Evaluates to double (25.0) because of upcasting
  ASSERT_TRUE(std::holds_alternative<double>(res));
  EXPECT_DOUBLE_EQ(std::get<double>(res), 25.0);
}
