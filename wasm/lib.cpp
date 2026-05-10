#include <string_view>

#ifdef __EMSCRIPTEN__
#include <emscripten/bind.h>
#include <emscripten/emscripten.h>
#define WASM_EXPORT EMSCRIPTEN_KEEPALIVE
#endif

#include "../lang/src/lexer/lexer.hpp"
// #include "../lang/src/parser/parser.hpp"

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

std::string tokenize(const char* source) {
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

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_BINDINGS(my_module) { emscripten::function("tokenize", &tokenize); }
#endif