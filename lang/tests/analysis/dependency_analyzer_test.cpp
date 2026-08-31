#include "dependency_analyzer.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <utility>

#include "types.hpp"

using namespace invariants::analysis;
using namespace invariants::binder;

namespace {

FieldSymbol makeField(const std::string& name,
                      ResolvedType type = ResolvedType{
                          invariants::ast::BuiltinType::Number}) {
  return FieldSymbol{.id = 0, .name = name, .resType = type};
}

BoundExprPtr makeAccess(const FieldSymbol* field, const std::string& path) {
  return std::make_unique<BoundExpr>(BoundFieldAccessExpr{field, path},
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
                              const std::string& target) {
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
  inv.constraints.push_back(BoundConstraint{
      .expr = std::move(expr), .isDeterministicPossible = false, .target = ""});
  return inv;
}

BoundExprPtr makeList(std::vector<BoundExprPtr> elements) {
  return std::make_unique<BoundExpr>(
      BoundListExpr{std::move(elements)},
      ResolvedType{invariants::ast::BuiltinType::Number});
}

}  // namespace

TEST(DependencyAnalyzerTest, ExtractsDeeplyNestedExpressions) {
  auto f0 = makeField("f0");
  auto f1 = makeField("f1");
  auto f2 = makeField("f2");
  auto f3 = makeField("f3");
  auto f4 = makeField("f4");
  auto f5 = makeField("f5");

  auto e1 = makeBinOp(makeAccess(&f0, "f0"), invariants::ast::BinaryOp::Add,
                      makeAccess(&f1, "f1"));
  auto e2 = makeBinOp(std::move(e1), invariants::ast::BinaryOp::Multiply,
                      makeAccess(&f2, "f2"));
  auto e3 = makeBinOp(std::move(e2), invariants::ast::BinaryOp::Greater,
                      makeAccess(&f3, "f3"));

  auto e4 = makeBinOp(makeAccess(&f4, "f4"), invariants::ast::BinaryOp::Less,
                      makeAccess(&f5, "f5"));
  auto e4_not = makeUnaryOp(invariants::ast::UnaryOp::Not, std::move(e4));

  auto root = makeBinOp(std::move(e3), invariants::ast::BinaryOp::Equal,
                        std::move(e4_not));

  auto deps = DependencyAnalyzer::extractDeps(*root, "");
  EXPECT_EQ(deps.size(), 6);
  EXPECT_TRUE(deps.contains("f0"));
  EXPECT_TRUE(deps.contains("f1"));
  EXPECT_TRUE(deps.contains("f2"));
  EXPECT_TRUE(deps.contains("f3"));
  EXPECT_TRUE(deps.contains("f4"));
  EXPECT_TRUE(deps.contains("f5"));
}

TEST(DependencyAnalyzerTest, SchedulesLongDependencyChain) {
  auto f0 = makeField("A");
  auto f1 = makeField("B");
  auto f2 = makeField("C");
  auto f3 = makeField("D");
  auto f4 = makeField("E");

  SpecSymbol rootSym{.id = 0, .name = "TestSpec"};
  BoundSpec spec;
  spec.symbol = &rootSym;

  spec.fields.push_back(BoundField{&f0});
  spec.fields.push_back(BoundField{&f1});
  spec.fields.push_back(BoundField{&f2});
  spec.fields.push_back(BoundField{&f3});
  spec.fields.push_back(BoundField{&f4});

  spec.invariants.push_back(
      makeAssignment("calc_E", makeAccess(&f3, "D"), "E"));
  spec.invariants.push_back(
      makeAssignment("calc_B", makeAccess(&f0, "A"), "B"));
  spec.invariants.push_back(
      makeAssignment("calc_D", makeAccess(&f2, "C"), "D"));
  spec.invariants.push_back(
      makeAssignment("calc_C", makeAccess(&f1, "B"), "C"));

  BoundModule module;
  module.specs.push_back(std::move(spec));

  DependencyAnalyzer analyzer;
  auto schedule = analyzer.analyze(module, "TestSpec");

  std::vector<std::string> expected = {"A", "B", "C", "D", "E"};
  EXPECT_EQ(schedule.order, expected);
}

TEST(DependencyAnalyzerTest, HandlesIndependentParallelChains) {
  auto f0 = makeField("f0");
  auto f1 = makeField("f1");
  auto f2 = makeField("f2");
  auto f3 = makeField("f3");
  auto f4 = makeField("f4");
  auto f5 = makeField("f5");

  SpecSymbol rootSym{.id = 0, .name = "TestSpec"};
  BoundSpec spec;
  spec.symbol = &rootSym;

  spec.fields.push_back(BoundField{&f0});
  spec.fields.push_back(BoundField{&f1});
  spec.fields.push_back(BoundField{&f2});
  spec.fields.push_back(BoundField{&f3});
  spec.fields.push_back(BoundField{&f4});
  spec.fields.push_back(BoundField{&f5});

  spec.invariants.push_back(
      makeAssignment("c1_step1", makeAccess(&f0, "f0"), "f1"));
  spec.invariants.push_back(
      makeAssignment("c1_step2", makeAccess(&f1, "f1"), "f2"));

  spec.invariants.push_back(
      makeAssignment("c2_step1", makeAccess(&f3, "f3"), "f4"));
  spec.invariants.push_back(
      makeAssignment("c2_step2", makeAccess(&f4, "f4"), "f5"));

  BoundModule module;
  module.specs.push_back(std::move(spec));

  DependencyAnalyzer analyzer;
  auto schedule = analyzer.analyze(module, "TestSpec");

  auto pos = [&](const std::string& id) {
    auto it = std::find(schedule.order.begin(), schedule.order.end(), id);
    return std::distance(schedule.order.begin(), it);
  };

  EXPECT_LT(pos("f0"), pos("f1"));
  EXPECT_LT(pos("f1"), pos("f2"));
  EXPECT_LT(pos("f3"), pos("f4"));
  EXPECT_LT(pos("f4"), pos("f5"));
}

TEST(DependencyAnalyzerTest, UnrollsAndFlattensNestedSpecs) {
  SpecSymbol userSym{.id = 1, .name = "UserSpec"};
  auto f_age = makeField("age");
  BoundSpec userSpec;
  userSpec.symbol = &userSym;
  userSpec.fields.push_back(BoundField{&f_age});

  SpecSymbol invoiceSym{.id = 2, .name = "InvoiceSpec"};
  auto f_client = makeField("client", ResolvedType{&userSym});
  auto f_total = makeField("total");
  BoundSpec invoiceSpec;
  invoiceSpec.symbol = &invoiceSym;
  invoiceSpec.fields.push_back(BoundField{&f_client});
  invoiceSpec.fields.push_back(BoundField{&f_total});

  auto calc_expr =
      makeBinOp(makeAccess(&f_age, "client.age"),
                invariants::ast::BinaryOp::Multiply, makeLiteral(2.0));
  invoiceSpec.invariants.push_back(
      makeAssignment("calc_total", std::move(calc_expr), "total"));

  BoundModule module;
  module.specs.push_back(std::move(userSpec));
  module.specs.push_back(std::move(invoiceSpec));

  DependencyAnalyzer analyzer;
  auto schedule = analyzer.analyze(module, "InvoiceSpec");

  std::vector<std::string> expected = {"client.age", "total"};
  EXPECT_EQ(schedule.order, expected);
}

TEST(DependencyAnalyzerTest, ThrowsOnComplexFourNodeCycle) {
  auto f0 = makeField("f0");
  auto f1 = makeField("f1");
  auto f2 = makeField("f2");
  auto f3 = makeField("f3");

  SpecSymbol rootSym{.id = 0, .name = "TestSpec"};
  BoundSpec spec;
  spec.symbol = &rootSym;
  spec.fields.push_back(BoundField{&f0});
  spec.fields.push_back(BoundField{&f1});
  spec.fields.push_back(BoundField{&f2});
  spec.fields.push_back(BoundField{&f3});

  spec.invariants.push_back(
      makeAssignment("inv1", makeAccess(&f0, "f0"), "f1"));
  spec.invariants.push_back(
      makeAssignment("inv2", makeAccess(&f1, "f1"), "f2"));
  spec.invariants.push_back(
      makeAssignment("inv3", makeAccess(&f2, "f2"), "f3"));
  spec.invariants.push_back(
      makeAssignment("inv4", makeAccess(&f3, "f3"), "f0"));

  BoundModule module;
  module.specs.push_back(std::move(spec));

  DependencyAnalyzer analyzer;
  EXPECT_THROW(analyzer.analyze(module, "TestSpec"), std::runtime_error);
}

TEST(DependencyAnalyzerTest, ThrowsOnDisjointGraphWithIsolatedCycle) {
  auto f0 = makeField("f0");
  auto f1 = makeField("f1");
  auto f2 = makeField("f2");
  auto f3 = makeField("f3");
  auto f4 = makeField("f4");
  auto f5 = makeField("f5");

  SpecSymbol rootSym{.id = 0, .name = "TestSpec"};
  BoundSpec spec;
  spec.symbol = &rootSym;
  spec.fields.push_back(BoundField{&f0});
  spec.fields.push_back(BoundField{&f1});
  spec.fields.push_back(BoundField{&f2});
  spec.fields.push_back(BoundField{&f3});
  spec.fields.push_back(BoundField{&f4});
  spec.fields.push_back(BoundField{&f5});

  spec.invariants.push_back(makeAssignment("v1", makeAccess(&f0, "f0"), "f1"));
  spec.invariants.push_back(makeAssignment("v2", makeAccess(&f1, "f1"), "f2"));

  spec.invariants.push_back(makeAssignment("b1", makeAccess(&f3, "f3"), "f4"));
  spec.invariants.push_back(makeAssignment("b2", makeAccess(&f4, "f4"), "f5"));
  spec.invariants.push_back(makeAssignment("b3", makeAccess(&f5, "f5"), "f3"));

  BoundModule module;
  module.specs.push_back(std::move(spec));

  DependencyAnalyzer analyzer;
  EXPECT_THROW(analyzer.analyze(module, "TestSpec"), std::runtime_error);
}

TEST(DependencyAnalyzerTest, SchedulesComplexTriggersCorrectly) {
  auto f0 = makeField("f0");
  auto f1 = makeField("f1");
  auto f2 = makeField("f2");
  auto f3 = makeField("f3");

  SpecSymbol rootSym{.id = 0, .name = "TestSpec"};
  BoundSpec spec;
  spec.symbol = &rootSym;
  spec.fields.push_back(BoundField{&f0});
  spec.fields.push_back(BoundField{&f1});
  spec.fields.push_back(BoundField{&f2});
  spec.fields.push_back(BoundField{&f3});

  spec.invariants.push_back(makeValidation(
      "T1", makeBinOp(makeAccess(&f0, "f0"), invariants::ast::BinaryOp::Less,
                      makeAccess(&f2, "f2"))));
  spec.invariants.push_back(makeValidation(
      "T2", makeBinOp(makeAccess(&f1, "f1"), invariants::ast::BinaryOp::Less,
                      makeAccess(&f3, "f3"))));
  spec.invariants.push_back(makeValidation(
      "T3", makeBinOp(makeAccess(&f0, "f0"), invariants::ast::BinaryOp::Less,
                      makeAccess(&f1, "f1"))));

  BoundModule module;
  module.specs.push_back(std::move(spec));

  DependencyAnalyzer analyzer;
  auto schedule = analyzer.analyze(module, "TestSpec");

  ASSERT_EQ(schedule.triggers["f2"].size(), 1);
  EXPECT_EQ(schedule.triggers["f2"][0].parentInv->name, "T1");

  ASSERT_EQ(schedule.triggers["f3"].size(), 1);
  EXPECT_EQ(schedule.triggers["f3"][0].parentInv->name, "T2");

  ASSERT_EQ(schedule.triggers["f1"].size(), 1);
  EXPECT_EQ(schedule.triggers["f1"][0].parentInv->name, "T3");

  EXPECT_EQ(schedule.triggers["f0"].size(), 0);
}

TEST(DependencyAnalyzerTest, TriggerFallsBackToFirstFieldIfNoDependencies) {
  auto f0 = makeField("f0");

  SpecSymbol rootSym{.id = 0, .name = "TestSpec"};
  BoundSpec spec;
  spec.symbol = &rootSym;
  spec.fields.push_back(BoundField{&f0});

  spec.invariants.push_back(makeValidation(
      "LiteralCheck",
      makeBinOp(makeLiteral(5.0), invariants::ast::BinaryOp::Equal,
                makeLiteral(5.0))));

  BoundModule module;
  module.specs.push_back(std::move(spec));

  DependencyAnalyzer analyzer;
  auto schedule = analyzer.analyze(module, "TestSpec");

  ASSERT_EQ(schedule.triggers["f0"].size(), 1);
  EXPECT_EQ(schedule.triggers["f0"][0].parentInv->name, "LiteralCheck");
}

TEST(DependencyAnalyzerTest, ExtractsDependenciesFromWithinLists) {
  auto f1 = makeField("f1");
  auto f2 = makeField("f2");

  std::vector<BoundExprPtr> elements;
  elements.push_back(makeAccess(&f1, "f1"));
  elements.push_back(makeLiteral(5.0));
  elements.push_back(makeAccess(&f2, "f2"));

  auto listExpr = makeList(std::move(elements));
  auto deps = DependencyAnalyzer::extractDeps(*listExpr, "");

  EXPECT_EQ(deps.size(), 2);
  EXPECT_TRUE(deps.contains("f1"));
  EXPECT_TRUE(deps.contains("f2"));
}