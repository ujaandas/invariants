#include "resolver.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "statements.hpp"

using namespace invariants::resolver;
namespace ast = invariants::ast;

namespace {

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