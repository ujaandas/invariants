#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "resolver.hpp"
#include "statements.hpp"
#include "types.hpp"

using namespace invariants::resolver;
namespace ast = invariants::ast;

namespace {

template <typename T, typename... Args>
std::vector<std::unique_ptr<T>> makePtrVector(Args&&... args) {
  std::vector<std::unique_ptr<T>> vec;
  vec.reserve(sizeof...(args));
  (vec.push_back(std::make_unique<T>(std::move(args))), ...);
  return vec;
}

ast::ConstraintStmt makeConstraint() {
  return ast::ConstraintStmt{.expression = std::make_unique<ast::Expr>(
                                 ast::LiteralExpr{.value = true})};
}

ast::FieldStmt makeFieldWithConstraints(const std::string& name) {
  return ast::FieldStmt{.identifier = name,
                        .type = std::make_unique<ast::Type>(
                            ast::SimpleType{.value = ast::BuiltinType::Number}),
                        .constraints = makePtrVector<ast::ConstraintStmt>(
                            makeConstraint(), makeConstraint())};
}

ast::InvariantStmt makeInvariant(const std::string& name) {
  return ast::InvariantStmt{
      .identifier = name,
      .constraints = makePtrVector<ast::ConstraintStmt>(makeConstraint())};
}

ast::SpecStmt makeSpecStmt(const std::string& name,
                           std::vector<ast::SpecMember> members = {}) {
  return ast::SpecStmt{.identifier = name, .members = std::move(members)};
}

ast::FieldStmt makeFieldStmt(const std::string& name) {
  return ast::FieldStmt{.identifier = name,
                        .type = std::make_unique<ast::Type>(
                            ast::SimpleType{.value = ast::BuiltinType::Number}),
                        .constraints = {}};
}

ast::FieldStmt makeCustomFieldStmt(const std::string& name,
                                   const std::string& typeName) {
  return ast::FieldStmt{
      .identifier = name,
      .type = std::make_unique<ast::Type>(ast::SimpleType{.value = typeName}),
      .constraints = {}};
}

template <typename... Args>
std::vector<ast::SpecMember> makeMembers(Args&&... args) {
  std::vector<ast::SpecMember> members;
  members.reserve(sizeof...(args));
  (members.push_back(ast::SpecMember(std::move(args))), ...);
  return members;
}

template <typename T>
ast::ConstraintStmt wrapExpr(T&& innerExpr) {
  return ast::ConstraintStmt{
      .expression = std::make_unique<ast::Expr>(std::forward<T>(innerExpr))};
}

}  // namespace

TEST(ResolverTest, ResolvingEmptySpecAddsToSymbolTable) {
  Resolver resolver;
  auto specStmt = makeSpecStmt("User");

  resolver(specStmt);

  const auto& table = resolver.getSt();
  const auto* specSymbol = table.lookup_spec("User");

  ASSERT_NE(specSymbol, nullptr);
  EXPECT_EQ(specSymbol->id, 0);
  EXPECT_EQ(specSymbol->name, "User");
  EXPECT_EQ(specSymbol->decl, &specStmt);
}

TEST(ResolverTest, ResolvingMultipleSpecsIncrementsSpecId) {
  Resolver resolver;
  auto firstSpec = makeSpecStmt("User");
  auto secondSpec = makeSpecStmt("Admin");

  resolver(firstSpec);
  resolver(secondSpec);

  const auto& table = resolver.getSt();
  const auto* userSymbol = table.lookup_spec("User");
  const auto* adminSymbol = table.lookup_spec("Admin");

  ASSERT_NE(userSymbol, nullptr);
  ASSERT_NE(adminSymbol, nullptr);
  EXPECT_EQ(userSymbol->id, 0);
  EXPECT_EQ(adminSymbol->id, 1);
}

TEST(ResolverTest, ResolvingModuleStmtResolvesAllIncludedSpecs) {
  Resolver resolver;

  auto spec1 = std::make_unique<ast::SpecStmt>(makeSpecStmt("User"));
  auto spec2 = std::make_unique<ast::SpecStmt>(makeSpecStmt("Post"));

  std::vector<std::unique_ptr<ast::SpecStmt>> specs;
  specs.push_back(std::move(spec1));
  specs.push_back(std::move(spec2));

  ast::ModuleStmt module{.specs = std::move(specs)};

  resolver(module);

  const auto& table = resolver.getSt();
  EXPECT_NE(table.lookup_spec("User"), nullptr);
  EXPECT_NE(table.lookup_spec("Post"), nullptr);
}

TEST(ResolverTest, ThrowsOnDuplicateSpecification) {
  Resolver resolver;
  auto spec1 = makeSpecStmt("User");
  auto spec2 = makeSpecStmt("User");

  resolver(spec1);

  EXPECT_THROW(resolver(spec2), std::runtime_error);
}

TEST(ResolverTest, ThrowsWhenFieldHasUnknownCustomType) {
  Resolver resolver;

  auto spec = makeSpecStmt(
      "User", makeMembers(makeCustomFieldStmt("location", "Address")));

  EXPECT_THROW(resolver(spec), std::runtime_error);
}

TEST(ResolverTest, ResolvesValidCustomType) {
  Resolver resolver;

  auto addressSpec = makeSpecStmt("Address");
  resolver(addressSpec);

  auto userSpec = makeSpecStmt(
      "User", makeMembers(makeCustomFieldStmt("location", "Address"),
                          makeFieldStmt("age")));

  EXPECT_NO_THROW(resolver(userSpec));
}

TEST(ResolverTest, StructuralStatementTraversalSucceeds) {
  Resolver resolver;

  auto spec = makeSpecStmt("BulkOrder",
                           makeMembers(makeFieldWithConstraints("quantity"),
                                       makeInvariant("valid_total_price")));

  EXPECT_NO_THROW(resolver(spec));
}

TEST(ResolverTest, SafelyIgnoresNullptrsInStatementVectors) {
  Resolver resolver;

  std::vector<ast::ConstraintPtr> corruptConstraints;
  corruptConstraints.push_back(nullptr);

  ast::InvariantStmt corruptInvariant{
      .identifier = "broken_invariant",
      .constraints = std::move(corruptConstraints)};

  auto spec =
      makeSpecStmt("BrokenSpec", makeMembers(std::move(corruptInvariant)));

  EXPECT_NO_THROW(resolver(spec));
}

TEST(ResolverTest, ImplicitValueKeywordSucceedsInFieldBlock) {
  Resolver resolver;

  // field quantity: Integer { value >= 1; }
  ast::FieldStmt field{
      .identifier = "quantity",
      .type = std::make_unique<ast::Type>(
          ast::SimpleType{.value = ast::BuiltinType::Integer}),
      .constraints = makePtrVector<ast::ConstraintStmt>(
          wrapExpr(ast::BinaryExpr{.left = std::make_unique<ast::Expr>(
                                       ast::IdentifierExpr{.name = "value"}),
                                   .op = ast::BinaryOp::GreaterEqual,
                                   .right = std::make_unique<ast::Expr>(
                                       ast::LiteralExpr{.value = 1.0})}))};

  ast::SpecStmt spec{.identifier = "Order",
                     .members = makeMembers(std::move(field))};
  EXPECT_NO_THROW(resolver(spec));
}

TEST(ResolverTest, ImplicitValueKeywordThrowsInInvariantBlock) {
  Resolver resolver;

  // invariant broken { value > 0; } -> invalid
  ast::InvariantStmt invalidInvariant{
      .identifier = "broken",
      .constraints = makePtrVector<ast::ConstraintStmt>(
          wrapExpr(ast::IdentifierExpr{.name = "value"}))};

  ast::SpecStmt spec{.identifier = "Order",
                     .members = makeMembers(std::move(invalidInvariant))};
  EXPECT_THROW(resolver(spec), std::runtime_error);
}

TEST(ResolverTest, MemberAccessResolvesCorrectlyWithThisKeywords) {
  Resolver resolver;

  // field price: Number
  ast::FieldStmt field{.identifier = "price",
                       .type = std::make_unique<ast::Type>(
                           ast::SimpleType{.value = ast::BuiltinType::Number})};

  // invariant valid { this.price > 0.0; }
  std::vector<ast::PostfixOp> ops;
  ops.push_back(ast::MemberAccessOp{.member = "price"});

  ast::InvariantStmt invariant{
      .identifier = "valid",
      .constraints = makePtrVector<ast::ConstraintStmt>(wrapExpr(
          ast::PostfixExpr{.base = std::make_unique<ast::Expr>(ast::ThisExpr{}),
                           .ops = std::move(ops)}))};

  ast::SpecStmt spec{
      .identifier = "Order",
      .members = makeMembers(std::move(field), std::move(invariant))};
  EXPECT_NO_THROW(resolver(spec));
}

TEST(ResolverTest, MemberAccessThrowsOnMissingField) {
  Resolver resolver;

  std::vector<ast::PostfixOp> ops;
  ops.push_back(ast::MemberAccessOp{.member = "unknown_field"});

  // invariant valid { this.unknown_field; }
  ast::InvariantStmt invariant{
      .identifier = "invalid_access",
      .constraints = makePtrVector<ast::ConstraintStmt>(wrapExpr(
          ast::PostfixExpr{.base = std::make_unique<ast::Expr>(ast::ThisExpr{}),
                           .ops = std::move(ops)}))};

  ast::SpecStmt spec{.identifier = "Order",
                     .members = makeMembers(std::move(invariant))};
  EXPECT_THROW(resolver(spec), std::runtime_error);
}

TEST(ResolverTest, GroupedThisMemberAccessResolvesCorrectly) {
  Resolver resolver;

  ast::FieldStmt field{.identifier = "price",
                       .type = std::make_unique<ast::Type>(
                           ast::SimpleType{.value = ast::BuiltinType::Number})};

  std::vector<ast::PostfixOp> ops;
  ops.push_back(ast::MemberAccessOp{.member = "price"});

  ast::InvariantStmt invariant{
      .identifier = "valid",
      .constraints = makePtrVector<ast::ConstraintStmt>(wrapExpr(
          ast::PostfixExpr{
              .base = std::make_unique<ast::Expr>(ast::GroupingExpr{
                  .expression = std::make_unique<ast::Expr>(
                      ast::GroupingExpr{.expression = std::make_unique<ast::Expr>(
                          ast::ThisExpr{})})}),
              .ops = std::move(ops)}))};

  ast::SpecStmt spec{
      .identifier = "Order",
      .members = makeMembers(std::move(field), std::move(invariant))};
  EXPECT_NO_THROW(resolver(spec));
}

TEST(ResolverTest, GroupedThisMemberAccessThrowsOnMissingField) {
  Resolver resolver;

  std::vector<ast::PostfixOp> ops;
  ops.push_back(ast::MemberAccessOp{.member = "unknown_field"});

  ast::InvariantStmt invariant{
      .identifier = "invalid_access",
      .constraints = makePtrVector<ast::ConstraintStmt>(wrapExpr(
          ast::PostfixExpr{.base = std::make_unique<ast::Expr>(ast::GroupingExpr{
                               .expression =
                                   std::make_unique<ast::Expr>(ast::ThisExpr{})}),
                           .ops = std::move(ops)}))};

  ast::SpecStmt spec{.identifier = "Order",
                     .members = makeMembers(std::move(invariant))};
  EXPECT_THROW(resolver(spec), std::runtime_error);
}

TEST(ResolverTest, ChainedMemberAccessResolvesThroughCustomSpecType) {
  Resolver resolver;

  ast::FieldStmt cityField{.identifier = "city",
                           .type = std::make_unique<ast::Type>(ast::SimpleType{
                               .value = ast::BuiltinType::String})};

  resolver(makeSpecStmt("Address", makeMembers(std::move(cityField))));

  std::vector<ast::PostfixOp> ops;
  ops.push_back(ast::MemberAccessOp{.member = "address"});
  ops.push_back(ast::MemberAccessOp{.member = "city"});

  ast::InvariantStmt invariant{
      .identifier = "valid",
      .constraints = makePtrVector<ast::ConstraintStmt>(wrapExpr(
          ast::PostfixExpr{.base = std::make_unique<ast::Expr>(ast::ThisExpr{}),
                           .ops = std::move(ops)}))};

  ast::SpecStmt user{
      .identifier = "User",
      .members = makeMembers(makeCustomFieldStmt("address", "Address"),
                             std::move(invariant))};

  EXPECT_NO_THROW(resolver(user));
}

TEST(ResolverTest, ChainedMemberAccessThrowsOnMissingNestedField) {
  Resolver resolver;

  ast::FieldStmt cityField{.identifier = "city",
                           .type = std::make_unique<ast::Type>(ast::SimpleType{
                               .value = ast::BuiltinType::String})};

  resolver(makeSpecStmt("Address", makeMembers(std::move(cityField))));

  std::vector<ast::PostfixOp> ops;
  ops.push_back(ast::MemberAccessOp{.member = "address"});
  ops.push_back(ast::MemberAccessOp{.member = "zipcode"});

  ast::InvariantStmt invariant{
      .identifier = "invalid",
      .constraints = makePtrVector<ast::ConstraintStmt>(wrapExpr(
          ast::PostfixExpr{.base = std::make_unique<ast::Expr>(ast::ThisExpr{}),
                           .ops = std::move(ops)}))};

  ast::SpecStmt user{
      .identifier = "User",
      .members = makeMembers(makeCustomFieldStmt("address", "Address"),
                             std::move(invariant))};

  EXPECT_THROW(resolver(user), std::runtime_error);
}