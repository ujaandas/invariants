#include "dependency_graph.hpp"

#include <gtest/gtest.h>

#include <stdexcept>

using namespace invariants::analysis;
using namespace invariants::binder;

TEST(DependencyGraphTest, InitializesEmptyGraph) {
  DependencyGraph graph(3);
  EXPECT_EQ(graph.size(), 3);

  auto order = graph.order();
  EXPECT_EQ(order.size(), 3);
}

TEST(DependencyGraphTest, ComputesLinearTopologicalOrder) {
  // 0 -> 1 -> 2
  DependencyGraph graph(3);
  graph.addEdge(0, 1);
  graph.addEdge(1, 2);

  auto order = graph.order();
  std::vector<FieldId> expected = {0, 1, 2};
  EXPECT_EQ(order, expected);
}

TEST(DependencyGraphTest, ComputesDiamondTopologicalOrder) {
  // 0 -> 1, 0 -> 2, 1 -> 3, 2 -> 3
  DependencyGraph graph(4);
  graph.addEdge(0, 1);
  graph.addEdge(0, 2);
  graph.addEdge(1, 3);
  graph.addEdge(2, 3);

  auto order = graph.order();
  ASSERT_EQ(order.size(), 4);
  EXPECT_EQ(order.front(), 0);
  EXPECT_EQ(order.back(), 3);
}

TEST(DependencyGraphTest, DeduplicatesRedundantEdges) {
  DependencyGraph graph(2);
  graph.addEdge(0, 1);
  graph.addEdge(0, 1);

  EXPECT_EQ(graph.getDependents(0).size(), 1);
  EXPECT_EQ(graph.getDependencies(1).size(), 1);
}

TEST(DependencyGraphTest, ThrowsOnDirectCycle) {
  // 0 -> 1 -> 0
  DependencyGraph graph(2);
  graph.addEdge(0, 1);
  graph.addEdge(1, 0);

  EXPECT_THROW(graph.order(), std::runtime_error);
}

TEST(DependencyGraphTest, ThrowsOnSelfLoop) {
  DependencyGraph graph(1);
  EXPECT_THROW(graph.addEdge(0, 0), std::runtime_error);
}

TEST(DependencyGraphTest, ThrowsOnOutOfBoundsAccess) {
  DependencyGraph graph(2);
  EXPECT_THROW(graph.addEdge(0, 5), std::out_of_range);
  EXPECT_THROW(graph.getDependents(3), std::out_of_range);
}