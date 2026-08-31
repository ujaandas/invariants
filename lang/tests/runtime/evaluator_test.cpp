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

TEST(EvaluatorTest, EvaluatesContextualValueKeyword) {
  std::string source = R"(
    spec Test {
      field age: Integer {
        value >= 18; // Testing the `value` keyword
      }
    }
  )";
  auto ast = parseSource(source);
  Binder binder;
  BoundModule bound = binder.bind(*ast);

  // fields[0] is `age`
  const auto& expr = bound.specs[0].fields[0].constraints[0].expr;
  Evaluator eval;

  // Simulate LLM guessing 20
  Environment envValid = {{"__value__", 20}};
  Value resValid = eval.evaluate(*expr, envValid);
  EXPECT_TRUE(std::get<bool>(resValid));

  // Simulate LLM guessing 16
  Environment envInvalid = {{"__value__", 16}};
  Value resInvalid = eval.evaluate(*expr, envInvalid);
  EXPECT_FALSE(std::get<bool>(resInvalid));
}

TEST(EvaluatorTest, ShortCircuitPreventsCrash) {
  std::string source = R"(
    spec Test {
      field qty: Number {}
      field safe_divide: Boolean {
        // Force == to be the root node by wrapping the logical block in parentheses
        value == ((this.qty != 0) && ((100 / this.qty) > 5.0));
      }
    }
  )";

  auto ast = parseSource(source);
  Binder binder;
  BoundModule bound = binder.bind(*ast);

  // fields[1] is `safe_divide`
  const auto& constraintExpr = bound.specs[0].fields[1].constraints[0].expr;
  const auto& equalityExpr = std::get<BoundBinaryExpr>(constraintExpr->value);
  const auto& logicExpr = equalityExpr.right;  // Get the `&&` part

  Evaluator eval;

  // Normal evaluation (qty = 10, 100/10 = 10 > 5 -> True)
  Environment envSafe = {{"qty", 10.0}};
  EXPECT_TRUE(std::get<bool>(eval.evaluate(*logicExpr, envSafe)));

  // Short-circuit evaluation (qty = 0)
  Environment envZero = {{"qty", 0.0}};
  // If short-circuiting fails, this will throw a Division-by-Zero exception!
  EXPECT_NO_THROW({
    Value res = eval.evaluate(*logicExpr, envZero);
    EXPECT_FALSE(std::get<bool>(res));
  });
}

TEST(EvaluatorTest, ListMembershipEvaluatesCorrectly) {
  std::string source = R"(
    spec Test {
      field opt_a: String {}
      field opt_b: String {}
      field currency: String {
        value IN [this.opt_a, this.opt_b, "GBP"];
      }
    }
  )";
  auto ast = parseSource(source);
  Binder binder;
  BoundModule bound = binder.bind(*ast);

  // fields[2] is `currency`
  const auto& expr = bound.specs[0].fields[2].constraints[0].expr;
  Evaluator eval;

  Environment env = {{"opt_a", std::string("USD")},
                     {"opt_b", std::string("EUR")},
                     // The LLM tries to emit "EUR"
                     {"__value__", std::string("EUR")}};

  Value res = eval.evaluate(*expr, env);
  EXPECT_TRUE(std::get<bool>(res));

  // The LLM tries to emit "JPY"
  env["__value__"] = std::string("JPY");
  res = eval.evaluate(*expr, env);
  EXPECT_FALSE(std::get<bool>(res));
}

TEST(EvaluatorTest, EvaluatesFlattenedNestedPaths) {
  std::string source = R"(
    spec User {
      field age: Integer { }
    }
    spec Invoice {
      field client: User { }
      field is_adult: Boolean {
        // Requires client.age to resolve from the environment map
        value == (this.client.age >= 18);
      }
    }
  )";

  auto ast = parseSource(source);
  Binder binder;
  BoundModule bound = binder.bind(*ast);

  // Extract `this.client.age >= 18`
  const auto& constraintExpr = bound.specs[1].fields[1].constraints[0].expr;
  const auto& equalityExpr = std::get<BoundBinaryExpr>(constraintExpr->value);
  const auto& mathExpr = equalityExpr.right;

  // Flattens nested JSON to strings automatically
  Environment envAdult = {{"client.age", 25}};

  Evaluator eval;
  Value resAdult = eval.evaluate(*mathExpr, envAdult);
  EXPECT_TRUE(std::get<bool>(resAdult));

  Environment envMinor = {{"client.age", 16}};
  Value resMinor = eval.evaluate(*mathExpr, envMinor);
  EXPECT_FALSE(std::get<bool>(resMinor));
}