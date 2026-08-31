#include "runtime.hpp"

#include <gtest/gtest.h>

#include <string>

#include "binder.hpp"
#include "dependency_analyzer.hpp"
#include "lexer.hpp"
#include "parser.hpp"

using namespace invariants::lexer;
using namespace invariants::parser;
using namespace invariants::binder;
using namespace invariants::analysis;
using namespace invariants::runtime;
using namespace invariants::ast;

namespace {

ModulePtr parseSource(const std::string& source) {
  Lexer lexer(source);
  auto tokens = lexer.scanTokens();

  Parser parser(tokens);
  auto module = parser.parseModule();

  EXPECT_NE(module, nullptr);
  return module;
}

}  // namespace

TEST(RuntimeSrcTest, ExecutesDeterministicAssignmentsAndValidations) {
  std::string source = R"(
    spec Order {
      field price: Number {}
      field qty: Number {}
      field total: Number {
        value == this.price * this.qty;
      }
      invariant max_budget {
        this.total <= 500.0;
      }
    }
  )";

  auto ast = parseSource(source);
  Binder binder;
  BoundModule bound = binder.bind(*ast);

  DependencyAnalyzer analyzer;
  auto schedule = analyzer.analyze(bound, "Order");

  Runtime runtime(bound, schedule);

  // Process 'price' (non-deterministic)
  ASSERT_TRUE(runtime.hasMoreFields());
  EXPECT_EQ(runtime.getActiveFieldName(), "price");
  EXPECT_FALSE(runtime.isActiveFieldDeterministic());

  // Test partial validation on price: negative price should fail field
  // constraint if any, or pass if none
  EXPECT_EQ(runtime.validatePartial("50.0"), ValidationStatus::Valid);
  runtime.submitValStr("price", "50.0");

  // Process 'qty' (non-deterministic)
  ASSERT_TRUE(runtime.hasMoreFields());
  EXPECT_EQ(runtime.getActiveFieldName(), "qty");
  EXPECT_FALSE(runtime.isActiveFieldDeterministic());
  runtime.submitValStr("qty", "10");

  // Process 'total' (deterministic assignment via `value == price * qty`)
  ASSERT_TRUE(runtime.hasMoreFields());
  EXPECT_EQ(runtime.getActiveFieldName(), "total");
  EXPECT_TRUE(runtime.isActiveFieldDeterministic());

  // Automatically solves total = 50.0 * 10 = 500.0, commits it, and triggers
  // `max_budget` (500 <= 500 -> Valid)
  std::string computedTotal = runtime.solveDeterministic();
  EXPECT_EQ(computedTotal, "500.000000");

  // Verify completion
  EXPECT_FALSE(runtime.hasMoreFields());

  // Verify final environment state
  const auto& env = runtime.getEnvironment();
  EXPECT_DOUBLE_EQ(std::get<double>(env.at("price")), 50.0);
  EXPECT_DOUBLE_EQ(std::get<double>(env.at("qty")), 10.0);
  EXPECT_DOUBLE_EQ(std::get<double>(env.at("total")), 500.0);
}

TEST(RuntimeSrcTest, EnforcesPartialValidationRejections) {
  std::string source = R"(
    spec UserProfile {
      field age: Integer {
        value >= 18;
      }
    }
  )";

  auto ast = parseSource(source);
  Binder binder;
  BoundModule bound = binder.bind(*ast);

  DependencyAnalyzer analyzer;
  auto schedule = analyzer.analyze(bound, "UserProfile");

  Runtime runtime(bound, schedule);

  ASSERT_TRUE(runtime.hasMoreFields());
  EXPECT_EQ(runtime.getActiveFieldName(), "age");

  // LLM attempts to generate an invalid underage token "16"
  EXPECT_EQ(runtime.validatePartial("16"), ValidationStatus::Invalid);

  // LLM attempts to generate a valid adult token "21"
  EXPECT_EQ(runtime.validatePartial("21"), ValidationStatus::Valid);

  // Commit valid generation
  runtime.submitValStr("age", "21");
  EXPECT_FALSE(runtime.hasMoreFields());

  EXPECT_EQ(std::get<int>(runtime.getEnvironment().at("age")), 21);
}

TEST(RuntimeSrcTest, KahnsAlgorithmForcesFieldReordering) {
  // Fields are declared in [a, b, c, d] order.
  // Dependencies: a relies on b & c. b relies on c. d relies on a.
  // Kahn's MUST reorder execution to: c -> b -> a -> d
  std::string source = R"(
    spec Topology {
      field a: Number { value == this.b + this.c; }
      field b: Number { value == this.c * 2.0; }
      field c: Number {}
      field d: Number { value == this.a + 10.0; }
    }
  )";

  auto ast = parseSource(source);
  Binder binder;
  BoundModule bound = binder.bind(*ast);
  DependencyAnalyzer analyzer;
  auto schedule = analyzer.analyze(bound, "Topology");
  Runtime runtime(bound, schedule);

  // 1. 'c' is the only independent variable
  EXPECT_EQ(runtime.getActiveFieldName(), "c");
  EXPECT_FALSE(runtime.isActiveFieldDeterministic());
  runtime.submitValStr("c", "5.0");

  // 2. 'b' solves next (5.0 * 2.0 = 10.0)
  EXPECT_EQ(runtime.getActiveFieldName(), "b");
  EXPECT_TRUE(runtime.isActiveFieldDeterministic());
  EXPECT_EQ(runtime.solveDeterministic(), "10.000000");

  // 3. 'a' solves next (10.0 + 5.0 = 15.0)
  EXPECT_EQ(runtime.getActiveFieldName(), "a");
  EXPECT_TRUE(runtime.isActiveFieldDeterministic());
  EXPECT_EQ(runtime.solveDeterministic(), "15.000000");

  // 4. 'd' solves last (15.0 + 10.0 = 25.0)
  EXPECT_EQ(runtime.getActiveFieldName(), "d");
  EXPECT_TRUE(runtime.isActiveFieldDeterministic());
  EXPECT_EQ(runtime.solveDeterministic(), "25.000000");

  EXPECT_FALSE(runtime.hasMoreFields());
}