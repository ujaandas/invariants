#include "dependency_analyzer.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>
#include <utility>

using namespace invariants::analysis;
using namespace invariants::binder;

namespace {

FieldSymbol makeField(FieldId id, const std::string& name = "") {
  return FieldSymbol{
      .id = id,
      .name = name,
      .resType = ResolvedType{invariants::ast::BuiltinType::Number}};
}

BoundExprPtr makeAccess(const FieldSymbol* field) {
  return std::make_unique<BoundExpr>(BoundFieldAccessExpr{field},
                                     field->resType);
}

BoundExprPtr makeLiteral(double val) {
  return std::make_unique<BoundExpr>(
      BoundLiteralExpr{val},
      ResolvedType{invariants::ast::BuiltinType::Number});
}

BoundExprPtr makeBinOp(BoundExprPtr left, invariants::ast::BinaryOp op,
                       BoundExprPtr right) {
  return std::make_unique<BoundExpr>(
      BoundBinaryExpr{std::move(left), op, std::move(right)},
      ResolvedType{invariants::ast::BuiltinType::Boolean});
}

BoundExprPtr makeUnaryOp(invariants::ast::UnaryOp op, BoundExprPtr operand) {
  return std::make_unique<BoundExpr>(
      BoundUnaryExpr{op, std::move(operand)},
      ResolvedType{invariants::ast::BuiltinType::Boolean});
}

BoundInvariant makeAssignment(std::string name, BoundExprPtr expr,
                              const FieldSymbol* target) {
  BoundInvariant inv;
  inv.name = std::move(name);
  inv.constraints.push_back(BoundConstraint{.expr = std::move(expr),
                                            .isDeterministicPossible = true,
                                            .target = target});
  return inv;
}

BoundInvariant makeValidation(std::string name, BoundExprPtr expr) {
  BoundInvariant inv;
  inv.name = std::move(name);
  inv.constraints.push_back(BoundConstraint{.expr = std::move(expr),
                                            .isDeterministicPossible = false,
                                            .target = nullptr});
  return inv;
}

}  // namespace

TEST(DependencyAnalyzerTest, ExtractsDeeplyNestedExpressions) {
  // (((f0 + f1) * f2) > f3) == !(f4 < f5)
  auto f0 = makeField(0);
  auto f1 = makeField(1);
  auto f2 = makeField(2);
  auto f3 = makeField(3);
  auto f4 = makeField(4);
  auto f5 = makeField(5);

  auto e1 = makeBinOp(makeAccess(&f0), invariants::ast::BinaryOp::Add,
                      makeAccess(&f1));
  auto e2 = makeBinOp(std::move(e1), invariants::ast::BinaryOp::Multiply,
                      makeAccess(&f2));
  auto e3 = makeBinOp(std::move(e2), invariants::ast::BinaryOp::Greater,
                      makeAccess(&f3));

  auto e4 = makeBinOp(makeAccess(&f4), invariants::ast::BinaryOp::Less,
                      makeAccess(&f5));
  auto e4_not = makeUnaryOp(invariants::ast::UnaryOp::Not, std::move(e4));

  auto root = makeBinOp(std::move(e3), invariants::ast::BinaryOp::Equal,
                        std::move(e4_not));

  auto deps = DependencyAnalyzer::extractDeps(*root);
  EXPECT_EQ(deps.size(), 6);
  for (FieldId i = 0; i <= 5; ++i) {
    EXPECT_TRUE(deps.contains(i));
  }
}

TEST(DependencyAnalyzerTest, SchedulesLongDependencyChain) {
  // E (4) depends on D (3) depends on C (2) depends on B (1) depends on A (0)
  auto f0 = makeField(0, "A");
  auto f1 = makeField(1, "B");
  auto f2 = makeField(2, "C");
  auto f3 = makeField(3, "D");
  auto f4 = makeField(4, "E");

  BoundSpec spec;
  // Intentionally add them out of order to ensure topological sort is doing the
  // work
  spec.invariants.push_back(makeAssignment("calc_E", makeAccess(&f3), &f4));
  spec.invariants.push_back(makeAssignment("calc_B", makeAccess(&f0), &f1));
  spec.invariants.push_back(makeAssignment("calc_D", makeAccess(&f2), &f3));
  spec.invariants.push_back(makeAssignment("calc_C", makeAccess(&f1), &f2));

  BoundModule module;
  module.specs.push_back(std::move(spec));

  DependencyAnalyzer analyzer;
  auto schedule = analyzer.analyze(module, 5);

  std::vector<FieldId> expected = {0, 1, 2, 3, 4};
  EXPECT_EQ(schedule.order, expected);
}

TEST(DependencyAnalyzerTest, HandlesIndependentParallelChains) {
  // Chain 1: 0 -> 1 -> 2
  // Chain 2: 3 -> 4 -> 5
  auto f0 = makeField(0);
  auto f1 = makeField(1);
  auto f2 = makeField(2);

  auto f3 = makeField(3);
  auto f4 = makeField(4);
  auto f5 = makeField(5);

  BoundSpec spec;
  spec.invariants.push_back(makeAssignment("c1_step1", makeAccess(&f0), &f1));
  spec.invariants.push_back(makeAssignment("c1_step2", makeAccess(&f1), &f2));

  spec.invariants.push_back(makeAssignment("c2_step1", makeAccess(&f3), &f4));
  spec.invariants.push_back(makeAssignment("c2_step2", makeAccess(&f4), &f5));

  BoundModule module;
  module.specs.push_back(std::move(spec));

  DependencyAnalyzer analyzer;
  auto schedule = analyzer.analyze(module, 6);

  // Both chains must remain internally ordered
  // Kahn's algorithm will likely interleave them (e.g. 0, 3, 1, 4, 2, 5)
  // We just verify that within the result, 0 comes before 1, 1 before 2, etc.
  auto pos = [&](FieldId id) {
    auto it = std::find(schedule.order.begin(), schedule.order.end(), id);
    return std::distance(schedule.order.begin(), it);
  };

  EXPECT_LT(pos(0), pos(1));
  EXPECT_LT(pos(1), pos(2));
  EXPECT_LT(pos(3), pos(4));
  EXPECT_LT(pos(4), pos(5));
}

TEST(DependencyAnalyzerTest, HandlesCrossSpecDependencies) {
  // Simulates invariants in Spec B depending on fields in Spec A
  // SpecA: field 0
  // SpecB: field 1, field 2
  // SpecB.f1 = SpecA.f0 * 2
  // SpecB.f2 = SpecB.f1 + SpecA.f0
  auto f0 = makeField(0);
  auto f1 = makeField(1);
  auto f2 = makeField(2);

  BoundSpec specA;
  BoundSpec specB;

  auto b1_expr = makeBinOp(makeAccess(&f0), invariants::ast::BinaryOp::Multiply,
                           makeLiteral(2.0));
  specB.invariants.push_back(
      makeAssignment("b1_calc", std::move(b1_expr), &f1));

  auto b2_expr = makeBinOp(makeAccess(&f1), invariants::ast::BinaryOp::Add,
                           makeAccess(&f0));
  specB.invariants.push_back(
      makeAssignment("b2_calc", std::move(b2_expr), &f2));

  BoundModule module;
  module.specs.push_back(std::move(specA));
  module.specs.push_back(std::move(specB));

  DependencyAnalyzer analyzer;
  auto schedule = analyzer.analyze(module, 3);

  std::vector<FieldId> expected = {0, 1, 2};
  EXPECT_EQ(schedule.order, expected);
}

TEST(DependencyAnalyzerTest, ThrowsOnComplexFourNodeCycle) {
  // A(0) -> B(1) -> C(2) -> D(3) -> A(0)
  auto f0 = makeField(0);
  auto f1 = makeField(1);
  auto f2 = makeField(2);
  auto f3 = makeField(3);

  BoundSpec spec;
  spec.invariants.push_back(makeAssignment("inv1", makeAccess(&f0), &f1));
  spec.invariants.push_back(makeAssignment("inv2", makeAccess(&f1), &f2));
  spec.invariants.push_back(makeAssignment("inv3", makeAccess(&f2), &f3));
  spec.invariants.push_back(makeAssignment("inv4", makeAccess(&f3), &f0));

  BoundModule module;
  module.specs.push_back(std::move(spec));

  DependencyAnalyzer analyzer;
  EXPECT_THROW(analyzer.analyze(module, 4), std::runtime_error);
}

TEST(DependencyAnalyzerTest, ThrowsOnDisjointGraphWithIsolatedCycle) {
  // Valid chain: 0 -> 1 -> 2
  // Broken chain: 3 -> 4 -> 5 -> 3
  auto f0 = makeField(0);
  auto f1 = makeField(1);
  auto f2 = makeField(2);
  auto f3 = makeField(3);
  auto f4 = makeField(4);
  auto f5 = makeField(5);

  BoundSpec spec;
  spec.invariants.push_back(makeAssignment("v1", makeAccess(&f0), &f1));
  spec.invariants.push_back(makeAssignment("v2", makeAccess(&f1), &f2));

  spec.invariants.push_back(makeAssignment("b1", makeAccess(&f3), &f4));
  spec.invariants.push_back(makeAssignment("b2", makeAccess(&f4), &f5));
  spec.invariants.push_back(makeAssignment("b3", makeAccess(&f5), &f3));

  BoundModule module;
  module.specs.push_back(std::move(spec));

  DependencyAnalyzer analyzer;
  EXPECT_THROW(analyzer.analyze(module, 6), std::runtime_error);
}

TEST(DependencyAnalyzerTest, SchedulesComplexTriggersCorrectly) {
  // Since there are no assignment invariants, the graph has no edges
  // Generation order defaults to [0, 1, 2, 3] due to Kahn's 0-indexed loop
  // We will add 3 validation invariants:
  // T1: 0 < 2   -> Should trigger on 2 (latest)
  // T2: 1 < 3   -> Should trigger on 3 (latest)
  // T3: 0 < 1   -> Should trigger on 1 (latest)
  auto f0 = makeField(0);
  auto f1 = makeField(1);
  auto f2 = makeField(2);
  auto f3 = makeField(3);

  BoundSpec spec;
  spec.invariants.push_back(makeValidation(
      "T1", makeBinOp(makeAccess(&f0), invariants::ast::BinaryOp::Less,
                      makeAccess(&f2))));
  spec.invariants.push_back(makeValidation(
      "T2", makeBinOp(makeAccess(&f1), invariants::ast::BinaryOp::Less,
                      makeAccess(&f3))));
  spec.invariants.push_back(makeValidation(
      "T3", makeBinOp(makeAccess(&f0), invariants::ast::BinaryOp::Less,
                      makeAccess(&f1))));

  BoundModule module;
  module.specs.push_back(std::move(spec));

  DependencyAnalyzer analyzer;
  auto schedule = analyzer.analyze(module, 4);

  // Assert triggers landed on the exact correct late-bound fields
  ASSERT_EQ(schedule.triggers[2].size(), 1);
  EXPECT_EQ(schedule.triggers[2][0].parentInv->name, "T1");

  ASSERT_EQ(schedule.triggers[3].size(), 1);
  EXPECT_EQ(schedule.triggers[3][0].parentInv->name, "T2");

  ASSERT_EQ(schedule.triggers[1].size(), 1);
  EXPECT_EQ(schedule.triggers[1][0].parentInv->name, "T3");

  // Fields 0 should have no triggers as it's never the "latest" in a pair
  EXPECT_EQ(schedule.triggers[0].size(), 0);
}

TEST(DependencyAnalyzerTest, TriggerFallsBackToFirstFieldIfNoDependencies) {
  // Edge case: an invariant with only literal operations, no cross-field deps
  BoundSpec spec;
  spec.invariants.push_back(makeValidation(
      "LiteralCheck",
      makeBinOp(makeLiteral(5.0), invariants::ast::BinaryOp::Equal,
                makeLiteral(5.0))));

  BoundModule module;
  module.specs.push_back(std::move(spec));

  DependencyAnalyzer analyzer;
  auto schedule = analyzer.analyze(module, 1);

  // Should trigger immediately on the very first field generated
  ASSERT_EQ(schedule.triggers[0].size(), 1);
  EXPECT_EQ(schedule.triggers[0][0].parentInv->name, "LiteralCheck");
}