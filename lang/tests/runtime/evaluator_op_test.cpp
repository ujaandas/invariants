#include <gtest/gtest.h>

#include <functional>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

#include "binder.hpp"
#include "evaluator.hpp"
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

// Compiles a dummy field, extracts the exact mathematical or logical
// right-hand side of a `value == (expr)` constraint, and evaluates it.
Value evalRaw(const std::string& expectedType, const std::string& exprStr,
              const Environment& env = {}) {
  std::string source =
      "spec Test { "
      "  field i1: Integer { } field i2: Integer { } "
      "  field n1: Number { } field n2: Number { } "
      "  field b1: Boolean { } field b2: Boolean { } "
      "  field s1: String { } field s2: String { } "
      "  field dummy: " +
      expectedType + " { value == (" + exprStr +
      "); } "
      "}";

  auto ast = parseSource(source);
  Binder binder;
  BoundModule bound = binder.bind(*ast);

  // Extract the right side of the equality to bypass Boolean-only invariant
  // restrictions
  const auto& constraintExpr = bound.specs[0].fields.back().constraints[0].expr;
  const auto& equalityExpr = std::get<BoundBinaryExpr>(constraintExpr->value);

  Evaluator evaluator;
  return evaluator.evaluate(*equalityExpr.right, env);
}

}  // namespace

struct EvalCase {
  std::string name;
  std::function<Value()> evaluate;
  Value expected;
};

void PrintTo(const EvalCase& value, std::ostream* os) { *os << value.name; }

class EvaluatorOpTest : public testing::TestWithParam<EvalCase> {};

TEST_P(EvaluatorOpTest, EvaluatesCorrectly) {
  const auto& param = GetParam();
  // gtest will natively compare std::variant underlying types and values
  EXPECT_EQ(param.evaluate(), param.expected);
}

INSTANTIATE_TEST_SUITE_P(
    EvaluatorOps, EvaluatorOpTest,
    testing::ValuesIn(std::vector<EvalCase>{
        // Unary
        {"unary_not_true",
         [] { return evalRaw("Boolean", "!this.b1", {{"b1", false}}); }, true},
        {"unary_not_false",
         [] { return evalRaw("Boolean", "!this.b1", {{"b1", true}}); }, false},
        {"unary_negate_int",
         [] { return evalRaw("Integer", "-this.i1", {{"i1", 42}}); }, -42},
        {"unary_negate_double",
         [] { return evalRaw("Number", "-this.n1", {{"n1", 3.14}}); }, -3.14},

        // Binary
        {"math_add_int",
         [] {
           return evalRaw("Integer", "this.i1 + this.i2",
                          {{"i1", 10}, {"i2", 5}});
         },
         15},
        {"math_add_upcast",
         [] {
           return evalRaw("Number", "this.i1 + this.n1",
                          {{"i1", 10}, {"n1", 5.5}});
         },
         15.5},
        {"math_add_string",
         [] {
           return evalRaw("String", "this.s1 + this.s2",
                          {{"s1", std::string("a")}, {"s2", std::string("b")}});
         },
         std::string("ab")},

        {"math_sub_int",
         [] {
           return evalRaw("Integer", "this.i1 - this.i2",
                          {{"i1", 10}, {"i2", 5}});
         },
         5},
        {"math_sub_upcast",
         [] {
           return evalRaw("Number", "this.n1 - this.i1",
                          {{"n1", 10.5}, {"i1", 5}});
         },
         5.5},

        {"math_mul_int",
         [] {
           return evalRaw("Integer", "this.i1 * this.i2",
                          {{"i1", 10}, {"i2", 5}});
         },
         50},
        {"math_mul_upcast",
         [] {
           return evalRaw("Number", "this.i1 * this.n1",
                          {{"i1", 2}, {"n1", 2.5}});
         },
         5.0},

        {"math_div_int",
         [] {
           return evalRaw("Integer", "this.i1 / this.i2",
                          {{"i1", 10}, {"i2", 3}});
         },
         3},  // Integer truncation
        {"math_div_upcast",
         [] {
           return evalRaw("Number", "this.i1 / this.n1",
                          {{"i1", 10}, {"n1", 4.0}});
         },
         2.5},

        // Relational
        {"rel_greater_true",
         [] {
           return evalRaw("Boolean", "this.n1 > this.i1",
                          {{"n1", 5.5}, {"i1", 5}});
         },
         true},
        {"rel_less_false",
         [] {
           return evalRaw("Boolean", "this.i1 < this.i2",
                          {{"i1", 10}, {"i2", 5}});
         },
         false},
        {"rel_greater_equal_true",
         [] {
           return evalRaw("Boolean", "this.i1 >= this.n1",
                          {{"i1", 10}, {"n1", 10.0}});
         },
         true},  // Cross-type equality
        {"rel_less_equal_true",
         [] {
           return evalRaw("Boolean", "this.i1 <= this.i2",
                          {{"i1", 5}, {"i2", 5}});
         },
         true},

        // Equality
        {"eq_numeric_cross_type",
         [] {
           return evalRaw("Boolean", "this.i1 == this.n1",
                          {{"i1", 5}, {"n1", 5.0}});
         },
         true},
        {"eq_string_true",
         [] {
           return evalRaw("Boolean", "this.s1 == \"test\"",
                          {{"s1", std::string("test")}});
         },
         true},
        {"neq_bool_true",
         [] {
           return evalRaw("Boolean", "this.b1 != this.b2",
                          {{"b1", true}, {"b2", false}});
         },
         true},

        // Boolean logic/shortcirc
        {"log_and_false",
         [] {
           return evalRaw("Boolean", "this.b1 && this.b2",
                          {{"b1", true}, {"b2", false}});
         },
         false},
        {"log_or_true",
         [] {
           return evalRaw("Boolean", "this.b1 || this.b2",
                          {{"b1", false}, {"b2", true}});
         },
         true},
        {"log_imply_true",
         [] {
           return evalRaw("Boolean", "this.b1 -> this.b2",
                          {{"b1", false}, {"b2", false}});
         },
         true},  // False implies False == True
        {"log_imply_false",
         [] {
           return evalRaw("Boolean", "this.b1 -> this.b2",
                          {{"b1", true}, {"b2", false}});
         },
         false},  // True implies False == False

        {"log_short_circuit_and",
         [] {
           return evalRaw("Boolean", "this.b1 && (10 / this.i1 > 0)",
                          {{"b1", false}, {"i1", 0}});
         },
         false},  // No div-by-zero throw
        {"log_short_circuit_or",
         [] {
           return evalRaw("Boolean", "this.b1 || (10 / this.i1 > 0)",
                          {{"b1", true}, {"i1", 0}});
         },
         true},  // No div-by-zero throw

        // List ops
        {"list_in_string",
         [] {
           return evalRaw("Boolean", "this.s1 IN [\"A\", \"B\"]",
                          {{"s1", std::string("A")}});
         },
         true},
        {"list_nin_string",
         [] {
           return evalRaw("Boolean", "this.s1 NIN [\"A\", \"B\"]",
                          {{"s1", std::string("C")}});
         },
         true},
        {"list_in_upcast",
         [] {
           return evalRaw("Boolean", "this.n1 IN [1.5, 2.0]", {{"n1", 2}});
         },
         true},  // Int 2 IN [Double 2.0]

        // Context
        {"contextual_value",
         [] { return evalRaw("Boolean", "value > 10", {{"__value__", 15}}); },
         true},
    }),
    [](const testing::TestParamInfo<EvalCase>& info) {
      return info.param.name;
    });