#include <gtest/gtest.h>

#include <stdexcept>
#include <string>

#include "binder.hpp"
#include "dependency_analyzer.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "symbol_table.hpp"

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
  auto schedule =
      analyzer.analyze(bound, binder.getSt().get_total_field_count());

  // Generation order should be linear [0, 1] as there are no assignment edges
  ASSERT_EQ(schedule.order.size(), 2);
  EXPECT_EQ(schedule.order[0], 0);  // start_date
  EXPECT_EQ(schedule.order[1], 1);  // end_date

  // The validation should trigger on the LATEST field evaluated (end_date, id:
  // 1)
  ASSERT_EQ(schedule.triggers[1].size(), 1);
  EXPECT_EQ(schedule.triggers[1][0].parentInv->name, "date_logic");
  EXPECT_TRUE(schedule.triggers[0].empty());  // start_date has no triggers
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
  auto schedule =
      analyzer.analyze(bound, binder.getSt().get_total_field_count());

  // Kahn's algorithm will emit [0, 1, 2, 3]
  // The expression references a(0), b(1), c(2), and d(3)
  // The latest field generated in the topological order is d(3)
  ASSERT_EQ(schedule.triggers[3].size(), 1);
  EXPECT_EQ(schedule.triggers[3][0].parentInv->name, "complex_check");

  EXPECT_TRUE(schedule.triggers[0].empty());
  EXPECT_TRUE(schedule.triggers[1].empty());
  EXPECT_TRUE(schedule.triggers[2].empty());
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
  // Subtotal (id 2) depends on Price (id 0) and Qty (id 1)
  ASSERT_TRUE(
      bound.specs[0].invariants[0].constraints[0].isDeterministicPossible);

  bound.specs[0].invariants[0].constraints[0].target =
      bound.specs[0].fields[2].symbol;

  // Total (id 3) depends on Subtotal (id 2)
  ASSERT_TRUE(
      bound.specs[0].invariants[1].constraints[0].isDeterministicPossible);

  bound.specs[0].invariants[1].constraints[0].target =
      bound.specs[0].fields[3].symbol;

  DependencyAnalyzer analyzer;
  auto schedule =
      analyzer.analyze(bound, binder.getSt().get_total_field_count());

  // Assert Topological Order: price and qty must precede subtotal. subtotal
  // must precede total
  auto getPos = [&](const std::string& name) {
    const FieldSymbol* field = binder.getSt().lookup_field("Invoice", name);
    auto it =
        std::find(schedule.order.begin(), schedule.order.end(), field->id);
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

  // Patch bounds to simulate assignment
  ASSERT_TRUE(
      bound.specs[0].invariants[0].constraints[0].isDeterministicPossible);

  bound.specs[0].invariants[0].constraints[0].target =
      bound.specs[0].fields[0].symbol;  // x

  ASSERT_TRUE(
      bound.specs[0].invariants[1].constraints[0].isDeterministicPossible);

  bound.specs[0].invariants[1].constraints[0].target =
      bound.specs[0].fields[1].symbol;  // y

  DependencyAnalyzer analyzer;

  // E2E Pipeline should successfully throw the cycle exception
  EXPECT_THROW(analyzer.analyze(bound, binder.getSt().get_total_field_count()),
               std::runtime_error);
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

  bound.specs[0].invariants[0].constraints[0].target =
      bound.specs[0].fields[1].symbol;  // derived

  DependencyAnalyzer analyzer;
  auto schedule =
      analyzer.analyze(bound, binder.getSt().get_total_field_count());

  // Generation order should put 'base' (0) before 'derived' (1). 'standalone'
  // (2) can be anywhere
  auto getPos = [&](FieldId id) {
    return std::distance(
        schedule.order.begin(),
        std::find(schedule.order.begin(), schedule.order.end(), id));
  };
  EXPECT_LT(getPos(0), getPos(1));

  // The standalone validation should trigger solely on the standalone field (id
  // 2)
  ASSERT_EQ(schedule.triggers[2].size(), 1);
  EXPECT_EQ(schedule.triggers[2][0].parentInv->name, "check_standalone");
}

TEST(DependencyAnalyzerIntegrationTest, AnalyzesComplexBulkOrderSpec) {
  std::string source = R"(
    spec BulkOrder {
      field unit_price: Number {
        value > 0.0;
      }
      field quantity: Integer {
        value >= 1;
        value <= 1000;
      }
      // Commented out while list binding is WIP
      // field currency: String {
      //  value IN ["USD", "EUR", "GBP"];
      // }
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

  // Verify Binder detected the assignment correctly
  const auto& valid_total = bound.specs[0].invariants[0].constraints[0];
  ASSERT_TRUE(valid_total.isDeterministicPossible);
  EXPECT_EQ(valid_total.target->name, "total_price");

  // Verify implication is correctly flagged as a validation, NOT an
  // assignment
  const auto& bulk_discount = bound.specs[0].invariants[1].constraints[0];
  ASSERT_FALSE(bulk_discount.isDeterministicPossible);
  EXPECT_EQ(bulk_discount.target, nullptr);

  DependencyAnalyzer analyzer;
  auto schedule =
      analyzer.analyze(bound, binder.getSt().get_total_field_count());

  auto getId = [&](const std::string& name) {
    return binder.getSt().lookup_field("BulkOrder", name)->id;
  };
  auto getPos = [&](const std::string& name) {
    auto it =
        std::find(schedule.order.begin(), schedule.order.end(), getId(name));
    return std::distance(schedule.order.begin(), it);
  };

  // Verify Topological Order (dependencies precede target)
  EXPECT_LT(getPos("unit_price"), getPos("total_price"));
  EXPECT_LT(getPos("quantity"), getPos("total_price"));

  // Verify Triggers
  // bulk_discount uses quantity, unit_price, and total_price.
  // Because total_price is guaranteed to be generated after the other two,
  // the trigger must land exactly on total_price.
  FieldId targetId = getId("total_price");
  ASSERT_EQ(schedule.triggers[targetId].size(), 1);
  EXPECT_EQ(schedule.triggers[targetId][0].parentInv->name, "bulk_discount");
}

TEST(DependencyAnalyzerIntegrationTest,
     AnalyzesCascadingMultiStageAssignments) {
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

  // Verify Binder detected both stages of the cascade
  ASSERT_TRUE(
      bound.specs[0].invariants[0].constraints[0].isDeterministicPossible);
  EXPECT_EQ(bound.specs[0].invariants[0].constraints[0].target->name,
            "tax_amount");

  ASSERT_TRUE(
      bound.specs[0].invariants[1].constraints[0].isDeterministicPossible);
  EXPECT_EQ(bound.specs[0].invariants[1].constraints[0].target->name,
            "net_salary");

  DependencyAnalyzer analyzer;
  auto schedule =
      analyzer.analyze(bound, binder.getSt().get_total_field_count());

  auto getPos = [&](const std::string& name) {
    const auto* field = binder.getSt().lookup_field("PayrollCalculation", name);
    auto it =
        std::find(schedule.order.begin(), schedule.order.end(), field->id);
    return std::distance(schedule.order.begin(), it);
  };

  // Verify cascading order constraints
  // Stage 1 dependencies
  EXPECT_LT(getPos("gross_salary"), getPos("tax_amount"));
  EXPECT_LT(getPos("tax_rate"), getPos("tax_amount"));

  // Stage 2 dependencies
  EXPECT_LT(getPos("tax_amount"), getPos("net_salary"));
  EXPECT_LT(getPos("bonus"), getPos("net_salary"));

  // Transitive dependency sanity check
  EXPECT_LT(getPos("gross_salary"), getPos("net_salary"));

  // Verify validation triggers on the absolute end of the cascade
  FieldId net_id =
      binder.getSt().lookup_field("PayrollCalculation", "net_salary")->id;
  ASSERT_EQ(schedule.triggers[net_id].size(), 1);
  EXPECT_EQ(schedule.triggers[net_id][0].parentInv->name, "check_take_home");
}

TEST(DependencyAnalyzerIntegrationTest,
     DoesNotFalselyFlagNestedEqualityAsAssignment) {
  std::string source = R"(
    spec EdgeCases {
      field a: Number { }
      field b: Number { }
      field c: Number { }

      // Root is `||`, NOT `==`. This should be a validation.
      invariant nested_equality {
        (this.a == this.b) || (this.c > 0.0);
      }

      // Root is `==`, but neither side is a pure field access. This is a validation.
      invariant math_equality {
        (this.a + 1.0) == (this.b + 2.0);
      }
    }
  )";

  auto ast = parseSource(source);
  Binder binder;
  BoundModule bound = binder.bind(*ast);

  // Both should be false, indicating pure validations
  ASSERT_FALSE(
      bound.specs[0].invariants[0].constraints[0].isDeterministicPossible);
  ASSERT_FALSE(
      bound.specs[0].invariants[1].constraints[0].isDeterministicPossible);

  DependencyAnalyzer analyzer;
  auto schedule =
      analyzer.analyze(bound, binder.getSt().get_total_field_count());

  // Generation order can be arbitrary [0, 1, 2] since there are no assignment
  // edges The analyzer should just attach the triggers to the latest fields
  // natively.
  FieldId last_field = schedule.order.back();

  // Because nested_equality touches a, b, c and math_equality touches a, b
  // We just ensure we didn't accidentally drop the triggers or throw a cycle
  EXPECT_GE(schedule.triggers[last_field].size(), 1);
}

TEST(DependencyAnalyzerIntegrationTest, AnalyzesComplexBulkOrderSpecWithLists) {
  std::string source = R"(
    spec BulkOrder {
      field unit_price: Number { value > 0.0; }
      field quantity: Integer { value >= 1; value <= 1000; }
      
      // List binding
      field currency: String {
        value IN ["USD", "EUR", "GBP"];
      }
      
      field total_price: Number { }

      invariant valid_total_price {
        this.total_price == this.unit_price * this.quantity;
      }
      invariant bulk_discount {
        !(this.quantity > 500) || (this.total_price < (this.unit_price * this.quantity));
      }
    }
  )";

  auto ast = parseSource(source);
  Binder binder;
  BoundModule bound = binder.bind(*ast);

  DependencyAnalyzer analyzer;
  auto schedule =
      analyzer.analyze(bound, binder.getSt().get_total_field_count());

  auto getId = [&](const std::string& name) {
    return binder.getSt().lookup_field("BulkOrder", name)->id;
  };
  auto getPos = [&](const std::string& name) {
    auto it =
        std::find(schedule.order.begin(), schedule.order.end(), getId(name));
    return std::distance(schedule.order.begin(), it);
  };

  // Core Topological Check still holds
  EXPECT_LT(getPos("unit_price"), getPos("total_price"));
  EXPECT_LT(getPos("quantity"), getPos("total_price"));

  // The discount trigger still lands precisely on total_price
  FieldId targetId = getId("total_price");
  ASSERT_EQ(schedule.triggers[targetId].size(), 1);
  EXPECT_EQ(schedule.triggers[targetId][0].parentInv->name, "bulk_discount");

  // The currency field's built-in IN constraint triggers exactly on itself
  FieldId currId = getId("currency");
  ASSERT_EQ(schedule.triggers[currId].size(), 1);
  // Constraints applied to fields directly don't have a named parent invariant
  EXPECT_EQ(schedule.triggers[currId][0].parentInv, nullptr);
}

TEST(DependencyAnalyzerIntegrationTest,
     ExtractsCrossFieldDependenciesInsideLists) {
  // This test proves that the dependency analyzer looks inside lists to find
  // field references
  std::string source = R"(
    spec CrossFieldList {
      field option_a: String { }
      field option_b: String { }
      
      field selection: String {
        // Selection must be one of the dynamically generated options
        value IN [this.option_a, this.option_b];
      }
    }
  )";

  auto ast = parseSource(source);
  Binder binder;
  BoundModule bound = binder.bind(*ast);

  DependencyAnalyzer analyzer;
  auto schedule =
      analyzer.analyze(bound, binder.getSt().get_total_field_count());

  auto getPos = [&](const std::string& name) {
    auto id = binder.getSt().lookup_field("CrossFieldList", name)->id;
    auto it = std::find(schedule.order.begin(), schedule.order.end(), id);
    return std::distance(schedule.order.begin(), it);
  };

  // Because selection depends on the list [this.option_a, this.option_b],
  // selection MUST be generated after both of them in the topological order!
  EXPECT_LT(getPos("option_a"), getPos("selection"));
  EXPECT_LT(getPos("option_b"), getPos("selection"));
}