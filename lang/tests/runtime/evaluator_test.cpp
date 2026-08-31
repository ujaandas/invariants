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

TEST(EvaluatorTest, EvaluatesUnaryOperators) {
  std::string source = R"(
    spec Test {
      field is_active: Boolean {}
      field balance: Number {}
      field valid: Boolean {
        // Tests negation (!) and unary minus (-)
        value == (!this.is_active && (-this.balance < 0.0));
      }
    }
  )";
  auto ast = parseSource(source);
  Binder binder;
  BoundModule bound = binder.bind(*ast);

  // fields[2] is `valid`
  const auto& constraintExpr = bound.specs[0].fields[2].constraints[0].expr;
  const auto& equalityExpr = std::get<BoundBinaryExpr>(constraintExpr->value);
  const auto& logicExpr = equalityExpr.right;

  Evaluator eval;

  // is_active = false (!false = true), balance = 50.0 (-50.0 < 0 = true)
  Environment env1 = {{"is_active", false}, {"balance", 50.0}};
  EXPECT_TRUE(std::get<bool>(eval.evaluate(*logicExpr, env1)));

  // is_active = true (!true = false) -> short circuits to false
  Environment env2 = {{"is_active", true}, {"balance", 50.0}};
  EXPECT_FALSE(std::get<bool>(eval.evaluate(*logicExpr, env2)));
}

TEST(EvaluatorTest, LogicalOrShortCircuits) {
  std::string source = R"(
    spec Test {
      field skip_check: Boolean {}
      field divisor: Number {}
      field safe_calc: Boolean {
        // If skip_check is true, division by zero is bypassed
        value == (this.skip_check || (100.0 / this.divisor > 1.0));
      }
    }
  )";
  auto ast = parseSource(source);
  Binder binder;
  BoundModule bound = binder.bind(*ast);

  const auto& constraintExpr = bound.specs[0].fields[2].constraints[0].expr;
  const auto& equalityExpr = std::get<BoundBinaryExpr>(constraintExpr->value);
  const auto& logicExpr = equalityExpr.right;

  Evaluator eval;

  // Normal evaluation (skip = false, divisor = 10)
  Environment envNormal = {{"skip_check", false}, {"divisor", 10.0}};
  EXPECT_TRUE(std::get<bool>(eval.evaluate(*logicExpr, envNormal)));

  // Short-circuit evaluation (skip = true, divisor = 0)
  Environment envZero = {{"skip_check", true}, {"divisor", 0.0}};
  EXPECT_NO_THROW({
    Value res = eval.evaluate(*logicExpr, envZero);
    EXPECT_TRUE(std::get<bool>(res));
  });
}

TEST(EvaluatorTest, EvaluatesStringEquality) {
  std::string source = R"(
    spec Test {
      field status: String {}
      field is_valid: Boolean {
        value == (this.status == "COMPLETED");
      }
    }
  )";
  auto ast = parseSource(source);
  Binder binder;
  BoundModule bound = binder.bind(*ast);

  const auto& constraintExpr = bound.specs[0].fields[1].constraints[0].expr;
  const auto& equalityExpr = std::get<BoundBinaryExpr>(constraintExpr->value);
  const auto& logicExpr = equalityExpr.right;

  Evaluator eval;

  Environment envMatch = {{"status", std::string("COMPLETED")}};
  EXPECT_TRUE(std::get<bool>(eval.evaluate(*logicExpr, envMatch)));

  Environment envMismatch = {{"status", std::string("PENDING")}};
  EXPECT_FALSE(std::get<bool>(eval.evaluate(*logicExpr, envMismatch)));
}

TEST(EvaluatorTest, ThrowsOnMissingEnvironmentVariable) {
  std::string source = R"(
    spec Test {
      field missing_field: Number {}
      field valid: Boolean {
        value == (this.missing_field > 10.0);
      }
    }
  )";
  auto ast = parseSource(source);
  Binder binder;
  BoundModule bound = binder.bind(*ast);

  const auto& constraintExpr = bound.specs[0].fields[1].constraints[0].expr;
  const auto& equalityExpr = std::get<BoundBinaryExpr>(constraintExpr->value);
  const auto& logicExpr = equalityExpr.right;

  Evaluator eval;

  // We intentionally pass an empty environment where `missing_field` does not
  // exist
  Environment envEmpty = {};

  // The evaluator should throw a runtime_error when it fails to resolve the
  // field access
  EXPECT_THROW({ eval.evaluate(*logicExpr, envEmpty); }, std::runtime_error);
}

TEST(EvaluatorTest, EvaluatesLogicalImplication) {
  std::string source = R"(
    spec Test {
      field is_premium: Boolean {}
      field discount: Number {
        // If premium, discount must be greater than 0
        this.is_premium -> (value > 0.0);
      }
    }
  )";
  auto ast = parseSource(source);
  Binder binder;
  BoundModule bound = binder.bind(*ast);

  // implication `->` is at the root of the expression for field 1
  const auto& logicExpr = bound.specs[0].fields[1].constraints[0].expr;

  Evaluator eval;

  // True -> True (Premium user with discount) == True
  Environment envValidPremium = {{"is_premium", true}, {"__value__", 15.0}};
  EXPECT_TRUE(std::get<bool>(eval.evaluate(*logicExpr, envValidPremium)));

  // True -> False (Premium user with NO discount) == False
  Environment envInvalidPremium = {{"is_premium", true}, {"__value__", 0.0}};
  EXPECT_FALSE(std::get<bool>(eval.evaluate(*logicExpr, envInvalidPremium)));

  // False -> False (Standard user with NO discount) == True (Vacuously true)
  Environment envStandard = {{"is_premium", false}, {"__value__", 0.0}};
  EXPECT_TRUE(std::get<bool>(eval.evaluate(*logicExpr, envStandard)));
}

TEST(EvaluatorTest, EvaluatesDeeplyNestedSpecPaths) {
  std::string source = R"(
    spec Level3 { field val: Integer {} }
    spec Level2 { field l3: Level3 {} }
    spec Level1 { field l2: Level2 {} }
    spec Root {
      field l1: Level1 {}
      field is_valid: Boolean {
        // Evaluates a 4-level deep dot notation path
        value == (this.l1.l2.l3.val == 100);
      }
    }
  )";
  auto ast = parseSource(source);
  Binder binder;
  BoundModule bound = binder.bind(*ast);

  // specs[3] is Root, fields[1] is is_valid
  const auto& constraintExpr = bound.specs[3].fields[1].constraints[0].expr;
  const auto& equalityExpr = std::get<BoundBinaryExpr>(constraintExpr->value);
  const auto& mathExpr = equalityExpr.right;

  Evaluator eval;

  // The Binder and Dependency Analyzer flatten the 4-level hop into a single
  // string key
  Environment envMatch = {{"l1.l2.l3.val", 100}};
  EXPECT_TRUE(std::get<bool>(eval.evaluate(*mathExpr, envMatch)));

  Environment envMismatch = {{"l1.l2.l3.val", 99}};
  EXPECT_FALSE(std::get<bool>(eval.evaluate(*mathExpr, envMismatch)));
}

TEST(EvaluatorTest, EvaluatesArithmeticAcrossMultipleSpecs) {
  std::string source = R"(
    spec Address { field zip: Integer {} }
    spec User {
      field age: Integer {}
      field addr: Address {}
    }
    spec Invoice {
      field client: User {}
      field multiplier: Number {}
      field total: Number {
        // Mixes flat fields with multi-hop nested fields in a single equation
        value == (this.client.age * this.multiplier) + this.client.addr.zip;
      }
    }
  )";
  auto ast = parseSource(source);
  Binder binder;
  BoundModule bound = binder.bind(*ast);

  // specs[2] is Invoice, fields[2] is total
  const auto& constraintExpr = bound.specs[2].fields[2].constraints[0].expr;
  const auto& equalityExpr = std::get<BoundBinaryExpr>(constraintExpr->value);
  const auto& mathExpr = equalityExpr.right;

  Evaluator eval;

  // client.age (20) * multiplier (1.5) = 30.0
  // 30.0 + client.addr.zip (90210) = 90240.0
  Environment env = {
      {"client.age", 20}, {"multiplier", 1.5}, {"client.addr.zip", 90210}};

  Value res = eval.evaluate(*mathExpr, env);

  ASSERT_TRUE(std::holds_alternative<double>(res));
  EXPECT_DOUBLE_EQ(std::get<double>(res), 90240.0);
}

TEST(EvaluatorTest, EvaluatesListMembershipWithNestedFields) {
  std::string source = R"(
    spec Preferences {
      field primary_color: String {}
      field secondary_color: String {}
    }
    spec User {
      field prefs: Preferences {}
      field chosen_color: String {
        // The list elements themselves are dynamically resolved from a nested spec
        value IN [this.prefs.primary_color, this.prefs.secondary_color, "black"];
      }
    }
  )";
  auto ast = parseSource(source);
  Binder binder;
  BoundModule bound = binder.bind(*ast);

  // specs[1] is User, fields[1] is chosen_color
  const auto& expr = bound.specs[1].fields[1].constraints[0].expr;
  Evaluator eval;

  // The generated JSON object will evaluate nested paths against the current
  // target value
  Environment env = {
      {"prefs.primary_color", std::string("red")},
      {"prefs.secondary_color", std::string("blue")},
      {"__value__", std::string("blue")}  // Match against a nested spec field
  };
  EXPECT_TRUE(std::get<bool>(eval.evaluate(*expr, env)));

  env["__value__"] = std::string("black");  // Match against a hardcoded literal
  EXPECT_TRUE(std::get<bool>(eval.evaluate(*expr, env)));

  env["__value__"] = std::string("green");  // No match
  EXPECT_FALSE(std::get<bool>(eval.evaluate(*expr, env)));
}

TEST(EvaluatorTest, EvaluatesCrossSpecImplications) {
  std::string source = R"(
    spec Configuration {
      field is_strict_mode: Boolean {}
      field max_retries: Integer {}
    }
    spec NetworkRequest {
      field config: Configuration {}
      field retries_attempted: Integer {
        // Implication condition spans into the nested spec
        this.config.is_strict_mode -> (value <= this.config.max_retries);
      }
    }
  )";
  auto ast = parseSource(source);
  Binder binder;
  BoundModule bound = binder.bind(*ast);

  // specs[1] is NetworkRequest, fields[1] is retries_attempted
  const auto& expr = bound.specs[1].fields[1].constraints[0].expr;
  Evaluator eval;

  // Strict mode is true, 4 <= 3 (False)
  Environment envViolated = {{"config.is_strict_mode", true},
                             {"config.max_retries", 3},
                             {"__value__", 4}};
  EXPECT_FALSE(std::get<bool>(eval.evaluate(*expr, envViolated)));

  // Strict mode is true, 2 <= 3 (True)
  Environment envPassed = {{"config.is_strict_mode", true},
                           {"config.max_retries", 3},
                           {"__value__", 2}};
  EXPECT_TRUE(std::get<bool>(eval.evaluate(*expr, envPassed)));

  // Strict mode is false, 4 <= 3 is bypassed (Vacuously True)
  Environment envBypassed = {{"config.is_strict_mode", false},
                             {"config.max_retries", 3},
                             {"__value__", 4}};
  EXPECT_TRUE(std::get<bool>(eval.evaluate(*expr, envBypassed)));
}