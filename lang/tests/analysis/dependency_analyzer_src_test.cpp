#include <gtest/gtest.h>

#include <algorithm>
#include <stdexcept>
#include <string>

#include "binder.hpp"
#include "dependency_analyzer.hpp"
#include "lexer.hpp"
#include "parser.hpp"

using namespace invariants::lexer;
using namespace invariants::parser;
using namespace invariants::binder;
using namespace invariants::analysis;
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

TEST(DependencyAnalyzerSrcTest, SchedulesLateBoundValidationTriggers) {
  std::string source = R"(
    spec Trip {
      field start_date: Number { }
      field end_date: Number { }
      
      invariant date_logic {
        this.start_date < this.end_date;
      }
    }
  )";

  auto ast = parseSource(source);
  Binder binder;
  BoundModule bound = binder.bind(*ast);

  DependencyAnalyzer analyzer;
  auto schedule = analyzer.analyze(bound, "Trip");

  // Generation order should be linear since there are no assignment edges
  ASSERT_EQ(schedule.order.size(), 2);
  EXPECT_EQ(schedule.order[0], "start_date");
  EXPECT_EQ(schedule.order[1], "end_date");

  // The validation should trigger on the LATEST field evaluated
  ASSERT_EQ(schedule.triggers["end_date"].size(), 1);
  EXPECT_EQ(schedule.triggers["end_date"][0].parentInv->name, "date_logic");
  EXPECT_TRUE(schedule.triggers["start_date"].empty());
}

TEST(DependencyAnalyzerSrcTest, AnalyzesDeeplyNestedExpressions) {
  std::string source = R"(
    spec MathLogic {
      field a: Number { }
      field b: Number { }
      field c: Number { }
      field d: Number { }

      invariant complex_check {
        ((this.a + this.b) * this.c) > (this.d / 2.0);
      }
    }
  )";

  auto ast = parseSource(source);
  Binder binder;
  BoundModule bound = binder.bind(*ast);

  DependencyAnalyzer analyzer;
  auto schedule = analyzer.analyze(bound, "MathLogic");

  // Kahn's algorithm will emit [a, b, c, d]
  // The expression references a, b, c, and d.
  // The latest field generated in the topological order is d
  ASSERT_EQ(schedule.triggers["d"].size(), 1);
  EXPECT_EQ(schedule.triggers["d"][0].parentInv->name, "complex_check");

  EXPECT_TRUE(schedule.triggers["a"].empty());
  EXPECT_TRUE(schedule.triggers["b"].empty());
  EXPECT_TRUE(schedule.triggers["c"].empty());
}

TEST(DependencyAnalyzerSrcTest, ComputesTopologicalOrderForAssignments) {
  std::string source = R"(
    spec Invoice {
      field price: Number { }
      field qty: Number { }
      field subtotal: Number { }
      field total: Number { }

      invariant calc_subtotal {
        this.subtotal == this.price * this.qty;
      }
      invariant calc_total {
        this.total == this.subtotal + 5.0;
      }
    }
  )";

  auto ast = parseSource(source);
  Binder binder;
  BoundModule bound = binder.bind(*ast);

  // MANUALLY patch the bound tree to simulate the Binder detecting assignments
  ASSERT_TRUE(
      bound.specs[0].invariants[0].constraints[0].isDeterministicPossible);

  ASSERT_TRUE(
      bound.specs[0].invariants[1].constraints[0].isDeterministicPossible);

  DependencyAnalyzer analyzer;
  auto schedule = analyzer.analyze(bound, "Invoice");

  auto getPos = [&](const std::string& name) {
    auto it = std::find(schedule.order.begin(), schedule.order.end(), name);
    return std::distance(schedule.order.begin(), it);
  };

  EXPECT_LT(getPos("price"), getPos("subtotal"));
  EXPECT_LT(getPos("qty"), getPos("subtotal"));
  EXPECT_LT(getPos("subtotal"), getPos("total"));
}

TEST(DependencyAnalyzerSrcTest, DetectsCyclesInAssignments) {
  std::string source = R"(
    spec CycleCheck {
      field x: Number { }
      field y: Number { }

      invariant calc_x {
        this.x == this.y + 1.0;
      }
      invariant calc_y {
        this.y == this.x + 1.0;
      }
    }
  )";

  auto ast = parseSource(source);
  Binder binder;
  BoundModule bound = binder.bind(*ast);

  ASSERT_TRUE(
      bound.specs[0].invariants[0].constraints[0].isDeterministicPossible);

  ASSERT_TRUE(
      bound.specs[0].invariants[1].constraints[0].isDeterministicPossible);

  DependencyAnalyzer analyzer;
  EXPECT_THROW(analyzer.analyze(bound, "CycleCheck"), std::runtime_error);
}

TEST(DependencyAnalyzerSrcTest, HandlesIndependentValidationsAndAssignments) {
  std::string source = R"(
    spec Mixed {
      field base: Number { }
      field derived: Number { }
      field standalone: Number { }

      invariant assign_derived {
        this.derived == this.base * 2.0;
      }

      invariant check_standalone {
        this.standalone > 0.0;
      }
    }
  )";

  auto ast = parseSource(source);
  Binder binder;
  BoundModule bound = binder.bind(*ast);

  // Mark only the assignment
  ASSERT_TRUE(
      bound.specs[0].invariants[0].constraints[0].isDeterministicPossible);

  DependencyAnalyzer analyzer;
  auto schedule = analyzer.analyze(bound, "Mixed");

  auto getPos = [&](const std::string& name) {
    return std::distance(
        schedule.order.begin(),
        std::find(schedule.order.begin(), schedule.order.end(), name));
  };

  EXPECT_LT(getPos("base"), getPos("derived"));

  ASSERT_EQ(schedule.triggers["standalone"].size(), 1);
  EXPECT_EQ(schedule.triggers["standalone"][0].parentInv->name,
            "check_standalone");
}

TEST(DependencyAnalyzerSrcTest, AnalyzesComplexBulkOrderSpec) {
  std::string source = R"(
    spec BulkOrder {
      field unit_price: Number { value > 0.0; }
      field quantity: Integer { value >= 1; value <= 1000; }
      field total_price: Number { }

      invariant valid_total_price {
        this.total_price == this.unit_price * this.quantity;
      }

      invariant bulk_discount {
        this.quantity > 500 -> this.total_price < (this.unit_price * this.quantity);
      }
    }
  )";

  auto ast = parseSource(source);
  Binder binder;
  BoundModule bound = binder.bind(*ast);

  const auto& valid_total = bound.specs[0].invariants[0].constraints[0];
  ASSERT_TRUE(valid_total.isDeterministicPossible);
  EXPECT_EQ(valid_total.target, "total_price");

  const auto& bulk_discount = bound.specs[0].invariants[1].constraints[0];
  ASSERT_FALSE(bulk_discount.isDeterministicPossible);
  EXPECT_EQ(bulk_discount.target, "");

  DependencyAnalyzer analyzer;
  auto schedule = analyzer.analyze(bound, "BulkOrder");

  auto getPos = [&](const std::string& name) {
    auto it = std::find(schedule.order.begin(), schedule.order.end(), name);
    return std::distance(schedule.order.begin(), it);
  };

  EXPECT_LT(getPos("unit_price"), getPos("total_price"));
  EXPECT_LT(getPos("quantity"), getPos("total_price"));

  // Ensure both triggers landed on total_price
  ASSERT_EQ(schedule.triggers["total_price"].size(), 2);

  bool has_assignment = false;
  bool has_validation = false;
  for (const auto& t : schedule.triggers["total_price"]) {
    if (t.parentInv->name == "valid_total_price") has_assignment = true;
    if (t.parentInv->name == "bulk_discount") has_validation = true;
  }
  EXPECT_TRUE(has_assignment);
  EXPECT_TRUE(has_validation);
}

TEST(DependencyAnalyzerSrcTest, AnalyzesCascadingMultiStageAssignments) {
  std::string source = R"(
    spec PayrollCalculation {
      field gross_salary: Number { }
      field tax_rate: Number { }
      field bonus: Number { }
      field tax_amount: Number { }
      field net_salary: Number { }

      invariant calc_tax {
        this.tax_amount == this.gross_salary * this.tax_rate;
      }
      invariant calc_net {
        this.net_salary == (this.gross_salary - this.tax_amount) + this.bonus;
      }
      invariant check_take_home {
        this.net_salary > 0.0;
      }
    }
  )";

  auto ast = parseSource(source);
  Binder binder;
  BoundModule bound = binder.bind(*ast);

  DependencyAnalyzer analyzer;
  auto schedule = analyzer.analyze(bound, "PayrollCalculation");

  auto getPos = [&](const std::string& name) {
    auto it = std::find(schedule.order.begin(), schedule.order.end(), name);
    return std::distance(schedule.order.begin(), it);
  };

  // Stage 1
  EXPECT_LT(getPos("gross_salary"), getPos("tax_amount"));
  EXPECT_LT(getPos("tax_rate"), getPos("tax_amount"));

  // Stage 2
  EXPECT_LT(getPos("tax_amount"), getPos("net_salary"));
  EXPECT_LT(getPos("bonus"), getPos("net_salary"));

  // Ensure both triggers landed on net_salary
  ASSERT_EQ(schedule.triggers["net_salary"].size(), 2);

  bool has_net_assignment = false;
  bool has_net_validation = false;
  for (const auto& t : schedule.triggers["net_salary"]) {
    if (t.parentInv->name == "calc_net") has_net_assignment = true;
    if (t.parentInv->name == "check_take_home") has_net_validation = true;
  }
  EXPECT_TRUE(has_net_assignment);
  EXPECT_TRUE(has_net_validation);
}

TEST(DependencyAnalyzerSrcTest, DoesNotFalselyFlagNestedEqualityAsAssignment) {
  std::string source = R"(
    spec EdgeCases {
      field a: Number { }
      field b: Number { }
      field c: Number { }

      invariant nested_equality { (this.a == this.b) || (this.c > 0.0); }
      invariant math_equality { (this.a + 1.0) == (this.b + 2.0); }
    }
  )";

  auto ast = parseSource(source);
  Binder binder;
  BoundModule bound = binder.bind(*ast);

  ASSERT_FALSE(
      bound.specs[0].invariants[0].constraints[0].isDeterministicPossible);
  ASSERT_FALSE(
      bound.specs[0].invariants[1].constraints[0].isDeterministicPossible);

  DependencyAnalyzer analyzer;
  auto schedule = analyzer.analyze(bound, "EdgeCases");
  std::string last_field = schedule.order.back();
  EXPECT_GE(schedule.triggers[last_field].size(), 1);
}

TEST(DependencyAnalyzerSrcTest, UnrollsDeeplyNestedSpecHierarchy) {
  std::string source = R"(
    spec Level3 { field val: Number { } }
    spec Level2 { field l3: Level3 { } }
    spec Level1 { field l2: Level2 { } }
    spec Root { field l1: Level1 { } }
  )";

  auto ast = parseSource(source);
  Binder binder;
  BoundModule bound = binder.bind(*ast);

  DependencyAnalyzer analyzer;
  auto schedule = analyzer.analyze(bound, "Root");

  ASSERT_EQ(schedule.order.size(), 1);
  EXPECT_EQ(schedule.order[0], "l1.l2.l3.val");
}

TEST(DependencyAnalyzerSrcTest, OrdersNestedAssignmentsCorrectly) {
  std::string source = R"(
    spec Address { field zip: Number { } }
    spec User { field addr: Address { } }
    spec Invoice {
      field client: User { }
      field total: Number { }
      invariant calc { this.total == this.client.addr.zip + 10.0; }
    }
  )";

  auto ast = parseSource(source);
  Binder binder;
  BoundModule bound = binder.bind(*ast);

  DependencyAnalyzer analyzer;
  auto schedule = analyzer.analyze(bound, "Invoice");

  auto getPos = [&](const std::string& name) {
    auto it = std::find(schedule.order.begin(), schedule.order.end(), name);
    return std::distance(schedule.order.begin(), it);
  };

  EXPECT_LT(getPos("client.addr.zip"), getPos("total"));
}

TEST(DependencyAnalyzerSrcTest, InterleavesMultipleInstantiations) {
  std::string source = R"(
    spec User { field age: Number { } }
    spec Match {
      field p1: User { }
      field p2: User { }
      field diff: Number { }
      
      invariant calc { this.diff == this.p1.age - this.p2.age; }
      invariant valid { this.p1.age > this.p2.age; }
    }
  )";

  auto ast = parseSource(source);
  Binder binder;
  BoundModule bound = binder.bind(*ast);

  DependencyAnalyzer analyzer;
  auto schedule = analyzer.analyze(bound, "Match");

  auto getPos = [&](const std::string& name) {
    auto it = std::find(schedule.order.begin(), schedule.order.end(), name);
    return std::distance(schedule.order.begin(), it);
  };

  // Ensure unique nodes were created and ordered properly
  EXPECT_LT(getPos("p1.age"), getPos("diff"));
  EXPECT_LT(getPos("p2.age"), getPos("diff"));

  // The validation should trigger on whichever age was generated last
  std::string laterAge =
      getPos("p1.age") > getPos("p2.age") ? "p1.age" : "p2.age";
  ASSERT_EQ(schedule.triggers[laterAge].size(), 1);
  EXPECT_EQ(schedule.triggers[laterAge][0].parentInv->name, "valid");
}

TEST(DependencyAnalyzerSrcTest, ResolvesNestedFieldConstraintsAndValueKeyword) {
  std::string source = R"(
    spec Child {
      field x: Number { value > 0.0; }
    }
    spec Parent {
      field c: Child { }
    }
  )";

  auto ast = parseSource(source);
  Binder binder;
  BoundModule bound = binder.bind(*ast);

  DependencyAnalyzer analyzer;
  auto schedule = analyzer.analyze(bound, "Parent");

  // Verify the flattened field exists
  ASSERT_EQ(schedule.order.size(), 1);
  EXPECT_EQ(schedule.order[0], "c.x");

  // Verify the field-level constraint was properly assigned to the unrolled
  // path
  ASSERT_EQ(schedule.triggers["c.x"].size(), 1);
  EXPECT_EQ(schedule.triggers["c.x"][0].parentInv,
            nullptr);  // Field constraints have no named parent
  EXPECT_EQ(schedule.triggers["c.x"][0].ownerFieldPath,
            "c.x");  // Used to inject `value` correctly
}

TEST(DependencyAnalyzerSrcTest, DetectsCrossSpecCycles) {
  // Invariants in parent create a cycle between nested fields
  std::string source = R"(
    spec Pair { field x: Number { } field y: Number { } }
    spec Container {
      field p: Pair { }
      invariant loop_x { this.p.x == this.p.y + 1.0; }
      invariant loop_y { this.p.y == this.p.x + 1.0; }
    }
  )";

  auto ast = parseSource(source);
  Binder binder;
  BoundModule bound = binder.bind(*ast);

  DependencyAnalyzer analyzer;
  EXPECT_THROW(analyzer.analyze(bound, "Container"), std::runtime_error);
}