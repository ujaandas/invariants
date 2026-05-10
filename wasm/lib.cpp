#include <emscripten/bind.h>
#include <emscripten/emscripten.h>

#include <string_view>

#include "lexer.hpp"
#include "parser.hpp"
#include "visitors/printer.hpp"

using namespace invariants;

namespace {
std::string escapeJson(std::string_view input) {
  std::string escaped;
  escaped.reserve(input.size());

  for (char c : input) {
    switch (c) {
      case '\\':
        escaped += "\\\\";
        break;
      case '"':
        escaped += "\\\"";
        break;
      case '\b':
        escaped += "\\b";
        break;
      case '\f':
        escaped += "\\f";
        break;
      case '\n':
        escaped += "\\n";
        break;
      case '\r':
        escaped += "\\r";
        break;
      case '\t':
        escaped += "\\t";
        break;
      default:
        escaped += c;
        break;
    }
  }

  return escaped;
}
}  // namespace

std::string tokenize(std::string source) {
  lexer::Lexer lexer(source);

  auto tokens = lexer.scanTokens();

  std::string out = R"({ "out": [ )";

  bool first = true;

  for (const auto& token : tokens) {
    if (!first) out += ", ";

    first = false;

    out += '"';
    out += escapeJson(token.toString());
    out += '"';
  }

  out += " ] }";

  return out;
}

std::string parse(std::string source) {
  try {
    lexer::Lexer lexer(source);
    auto tokens = lexer.scanTokens();

    parser::Parser parser(tokens);
    auto module = parser.parseModule();
    auto printed = ast::visitors::Printer{}.print(*module);

    std::string out = R"({ "out": " )";
    out += escapeJson(printed);
    out += R"( " })";

    return out;
  } catch (const std::exception& e) {
    return std::string(R"({ "error": ")") + escapeJson(e.what()) + R"(" })";
  }
}

EMSCRIPTEN_BINDINGS(my_module) {
  emscripten::function("tokenize", &tokenize);
  emscripten::function("parse", &parse);
}