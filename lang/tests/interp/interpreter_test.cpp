#include "interpreter.hpp"

#include <gtest/gtest.h>

#include <stdexcept>

using invariants::interpreter::Interpreter;

TEST(InterpreterTest, RunsSimpleInputWithoutThrowing) {
  Interpreter interp;

  EXPECT_NO_THROW(interp.run("spec User {}"));
}

TEST(InterpreterTest, PropagatesLexerErrorsForInvalidInput) {
  Interpreter interp;

  EXPECT_THROW(interp.run("@"), std::invalid_argument);
}
