#include <gtest/gtest.h>

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
  auto schedule =
      analyzer.analyze(bound, binder.getSt().get_total_field_count());

  // Generation order should be linear [0, 1] as there are no assignment edges
  ASSERT_EQ(schedule.order.size(), 2);
  EXPECT_EQ(schedule.order[0], 0);  // start_date
  EXPECT_EQ(schedule.order[1], 1);  // end_date

  // The validation should trigger on the LATEST field evaluated (end_date, id:
  // 1)
  ASSERT_EQ(schedule.triggers[1].size(), 1);
  EXPECT_EQ(schedule.triggers[1][0]->name, "date_logic");
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
  EXPECT_EQ(schedule.triggers[3][0]->name, "complex_check");

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
  bound.specs[0].invariants[0].isDeterministicPossible = true;
  bound.specs[0].invariants[0].target = bound.specs[0].fields[2].symbol;

  // Total (id 3) depends on Subtotal (id 2)
  bound.specs[0].invariants[1].isDeterministicPossible = true;
  bound.specs[0].invariants[1].target = bound.specs[0].fields[3].symbol;

  DependencyAnalyzer analyzer;
  auto schedule =
      analyzer.analyze(bound, binder.getSt().get_total_field_count());

  // Assert Topological Order: price and qty must precede subtotal. subtotal
  // must precede total
  auto get_pos = [&](const std::string& name) {
    const FieldSymbol* field = binder.getSt().lookup_field("Invoice", name);
    auto it =
        std::find(schedule.order.begin(), schedule.order.end(), field->id);
    return std::distance(schedule.order.begin(), it);
  };

  EXPECT_LT(get_pos("price"), get_pos("subtotal"));
  EXPECT_LT(get_pos("qty"), get_pos("subtotal"));
  EXPECT_LT(get_pos("subtotal"), get_pos("total"));
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
  bound.specs[0].invariants[0].isDeterministicPossible = true;
  bound.specs[0].invariants[0].target = bound.specs[0].fields[0].symbol;  // x

  bound.specs[0].invariants[1].isDeterministicPossible = true;
  bound.specs[0].invariants[1].target = bound.specs[0].fields[1].symbol;  // y

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
  bound.specs[0].invariants[0].isDeterministicPossible = true;
  bound.specs[0].invariants[0].target =
      bound.specs[0].fields[1].symbol;  // derived

  DependencyAnalyzer analyzer;
  auto schedule =
      analyzer.analyze(bound, binder.getSt().get_total_field_count());

  // Generation order should put 'base' (0) before 'derived' (1). 'standalone'
  // (2) can be anywhere
  auto get_pos = [&](FieldId id) {
    return std::distance(
        schedule.order.begin(),
        std::find(schedule.order.begin(), schedule.order.end(), id));
  };
  EXPECT_LT(get_pos(0), get_pos(1));

  // The standalone validation should trigger solely on the standalone field (id
  // 2)
  ASSERT_EQ(schedule.triggers[2].size(), 1);
  EXPECT_EQ(schedule.triggers[2][0]->name, "check_standalone");
}