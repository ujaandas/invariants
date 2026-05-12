#include "env.hpp"

#include <format>
#include <stdexcept>
#include <string>

#include "token.hpp"

using namespace invariants::env;

// cppcheck-suppress unusedPrivateFunction
void Environment::define(std::string name, ObjT value) {
  values.insert({name, value});
}

// cppcheck-suppress unusedPrivateFunction
ObjT Environment::get(const invariants::lexer::Token& token) {
  auto lex = token.getLexeme();
  if (values.contains(lex)) {
    return values.at(lex);
  }

  throw std::runtime_error(std::format("undefined variable {}", lex));
}