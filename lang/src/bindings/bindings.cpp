#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <cctype>
#include <iostream>
#include <limits>
#include <string_view>

#include "binder.hpp"
#include "dependency_analyzer.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "runtime.hpp"
#include "symbol_table.hpp"

using invariants::ast::BuiltinType;
using invariants::binder::ResolvedType;

namespace py = pybind11;
using namespace invariants::runtime;

// Strict JSON Integer: -?(0|[1-9][0-9]*)
inline bool isValidPartialJsonInteger(std::string_view s, bool& canExit) {
  canExit = false;
  if (s.empty()) return false;

  size_t idx = 0;
  if (s[idx] == '-') {
    idx++;
    if (idx == s.size()) {
      return true;  // "-" is a valid partial prefix, but CANNOT exit
    }
  }

  if (idx >= s.size() || !std::isdigit(static_cast<unsigned char>(s[idx]))) {
    return false;
  }

  if (s[idx] == '0') {
    idx++;
    if (idx < s.size()) {
      return false;  // Trailing digits after '0' are invalid in integers
    }
  } else {
    while (idx < s.size() && std::isdigit(static_cast<unsigned char>(s[idx]))) {
      idx++;
    }
  }

  if (idx != s.size()) return false;
  canExit = true;
  return true;
}

// Strict JSON Number: -?(0|[1-9][0-9]*)(\.[0-9]+)?([eE][+-]?[0-9]+)?
inline bool isValidPartialJsonNumber(std::string_view s, bool& canExit) {
  canExit = false;
  if (s.empty()) return false;

  size_t idx = 0;
  if (s[idx] == '-') {
    idx++;
    if (idx == s.size()) {
      return true;
    }
  }

  if (idx >= s.size() || !std::isdigit(static_cast<unsigned char>(s[idx]))) {
    return false;
  }

  if (s[idx] == '0') {
    idx++;
    if (idx < s.size() && std::isdigit(static_cast<unsigned char>(s[idx]))) {
      return false;
    }
  } else {
    while (idx < s.size() && std::isdigit(static_cast<unsigned char>(s[idx]))) {
      idx++;
    }
  }

  canExit = true;

  if (idx < s.size() && s[idx] == '.') {
    idx++;
    canExit = false;
    if (idx == s.size()) return true;
    if (!std::isdigit(static_cast<unsigned char>(s[idx]))) return false;
    while (idx < s.size() && std::isdigit(static_cast<unsigned char>(s[idx]))) {
      idx++;
    }
    canExit = true;
  }

  if (idx < s.size() && (s[idx] == 'e' || s[idx] == 'E')) {
    idx++;
    canExit = false;
    if (idx == s.size()) return true;

    if (s[idx] == '+' || s[idx] == '-') {
      idx++;
      if (idx == s.size()) return true;
    }
    if (!std::isdigit(static_cast<unsigned char>(s[idx]))) return false;
    while (idx < s.size() && std::isdigit(static_cast<unsigned char>(s[idx]))) {
      idx++;
    }
    canExit = true;
  }

  return idx == s.size();
}

// Strict JSON Boolean: "true" | "false"
inline bool isValidPartialJsonBoolean(std::string_view s, bool& canExit) {
  canExit = (s == "true" || s == "false");
  if (canExit) return true;

  constexpr std::string_view t = "true";
  constexpr std::string_view f = "false";
  if (s.size() < t.size() && t.substr(0, s.size()) == s) return true;
  if (s.size() < f.size() && f.substr(0, s.size()) == s) return true;

  return false;
}

// True if `s` ends in an ASCII whitespace char or a tokenizer-space marker
// (SentencePiece '\xE2\x96\x81' or BPE 'Ġ' = 0xC4 0xA0).
inline bool endsWithTokenizerSpace(std::string_view s) {
  if (s.empty()) return false;
  char last = s.back();
  if (last == ' ' || last == '\t' || last == '\r' || last == '\n') return true;
  if (s.size() >= 3 && s.substr(s.size() - 3) == "\xE2\x96\x81") return true;
  if (s.size() >= 2 && static_cast<unsigned char>(s[s.size() - 2]) == 0xC4 &&
      static_cast<unsigned char>(s[s.size() - 1]) == 0xA0) {
    return true;
  }
  return false;
}

// True if `s` is made up ENTIRELY of ASCII whitespace / tokenizer-space
// markers (i.e. stripping the same leading-whitespace sequence used
// elsewhere in this file consumes the whole string).
inline bool isPureTokenizerSpace(std::string_view s) {
  if (s.empty()) return false;
  size_t start_idx = 0;
  while (start_idx < s.size()) {
    unsigned char c = s[start_idx];
    if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
      start_idx++;
    } else if (start_idx + 2 < s.size() &&
               s.substr(start_idx, 3) == "\xE2\x96\x81") {
      start_idx += 3;
    } else if (c == 0xC4 && start_idx + 1 < s.size() &&
               static_cast<unsigned char>(s[start_idx + 1]) == 0xA0) {
      start_idx += 2;
    } else {
      break;
    }
  }
  return start_idx == s.size();
}

class EngineSession {
  std::unique_ptr<invariants::ast::ModuleStmt> ast;
  invariants::binder::Binder binder;
  invariants::binder::BoundModule boundModule;
  invariants::analysis::ExecutionSchedule schedule;
  std::unique_ptr<invariants::runtime::Runtime> runtime;

 public:
  EngineSession(const std::string& source, const std::string& rootSpec) {
    invariants::lexer::Lexer lexer(source);
    auto tokens = lexer.scanTokens();

    invariants::parser::Parser parser(tokens);
    ast = parser.parseModule();

    boundModule = binder.bind(*ast);

    invariants::analysis::DependencyAnalyzer analyzer;
    schedule = analyzer.analyze(boundModule, rootSpec);

    runtime =
        std::make_unique<invariants::runtime::Runtime>(boundModule, schedule);
  }

  invariants::runtime::Runtime& getRuntime() { return *runtime; }
};

PYBIND11_MODULE(invariants_cpp, m) {
  m.doc() = "Invariants LLM constrained execution runtime engine";

  py::enum_<ValidationStatus>(m, "ValidationStatus")
      .value("Valid", ValidationStatus::Valid)
      .value("PartialValid", ValidationStatus::PartialValid)
      .value("Invalid", ValidationStatus::Invalid)
      .export_values();

  py::class_<Runtime>(m, "Runtime")
      .def("has_more_fields", &Runtime::hasMoreFields)
      .def("get_active_field_name", &Runtime::getActiveFieldName)
      .def("is_active_field_deterministic",
           &Runtime::isActiveFieldDeterministic)
      .def("solve_deterministic", &Runtime::solveDeterministic)
      .def("submit_val_str", &Runtime::submitValStr)
      .def("validate_partial", &Runtime::validatePartial,
           py::arg("proposed_chars"), py::arg("is_complete") = true);

  py::class_<EngineSession>(m, "EngineSession")
      .def(py::init<const std::string&, const std::string&>(),
           py::arg("source"), py::arg("root_spec"))
      .def_property_readonly("runtime", &EngineSession::getRuntime,
                             py::return_value_policy::reference_internal);

  m.def(
      "mask_logits_full_vocab",
      [](const Runtime& rt, py::array_t<float>& logits,
         const std::vector<std::string>& vocab,
         const std::string& current_buffer, bool verbose = false) {
        py::buffer_info buf = logits.request();
        float* ptr = static_cast<float*>(buf.ptr);
        size_t vocab_size = vocab.size();

        auto active_sym = rt.getActiveFieldSymbol();
        ResolvedType field_type = active_sym->resType;

        if (verbose) {
          std::cout
              << "\n\n==================================================\n";
          std::cout << "[C++ Mask] STARTING MASK FOR FIELD: '"
                    << active_sym->name << "'\n";
          std::cout << "[C++ Mask] Current Buffer: '" << current_buffer
                    << "'\n";
        }

        bool isString = false, isInteger = false, isNumber = false,
             isBool = false;

        // Based on binder.hpp, ResolvedType is strictly the base type!
        // Constraints are external, so isBuiltin() is 100% accurate.
        if (field_type.isBuiltin()) {
          auto bt = std::get<BuiltinType>(field_type.type);
          isString = (bt == BuiltinType::String);
          isInteger = (bt == BuiltinType::Integer);
          isNumber = (bt == BuiltinType::Number);
          isBool = (bt == BuiltinType::Boolean);
          if (verbose)
            std::cout << "[C++ Mask] Base Type: Builtin (Integer=" << isInteger
                      << ")\n";
        } else {
          if (verbose)
            std::cout << "[C++ Mask] Base Type: Complex (Spec/Array/Map)\n";
        }

        int surviving_tokens = 0;

        for (size_t i = 0; i < vocab_size; ++i) {
          const std::string& token_str = vocab[i];
          if (token_str.empty()) {
            ptr[i] = -std::numeric_limits<float>::infinity();
            continue;
          }

          // Trace problematic tokens when verbose is True
          bool trace_token = false;
          if (verbose && (token_str.find("18") != std::string::npos ||
                          token_str.find("\xE6\x88\x90") !=
                              std::string::npos)) {  // \xE6\x88\x90 is "成"
            trace_token = true;
            std::cout << "\n[C++ Mask TRACE] Evaluating Token ID " << i << ": '"
                      << token_str << "'\n";
          }

          std::string proposed = current_buffer + token_str;
          std::string_view sv(proposed);

          // 0. MAX LENGTH SAFETY VALVE: no legitimate value here needs more
          // than a few dozen characters. Caps unbounded-padding patterns
          // (whitespace, redundant decimal/exponent digits, etc) that would
          // otherwise run until n_ctx. Tokens that close out the value are
          // still allowed through to normal validation.
          constexpr size_t kMaxOpenFieldValueLength = 40;
          if (current_buffer.size() >= kMaxOpenFieldValueLength &&
              sv.find_first_of(",}\n") == std::string_view::npos) {
            if (trace_token)
              std::cout << "   -> REJECTED: exceeded max open field-value "
                           "length ("
                        << kMaxOpenFieldValueLength << ").\n";
            ptr[i] = -std::numeric_limits<float>::infinity();
            continue;
          }

          // 1. STRICT BAN ON LEADING ASCII WHITESPACE
          if (current_buffer.empty()) {
            char first_c = sv.front();
            if (first_c == ' ' || first_c == '\t' || first_c == '\r' ||
                first_c == '\n') {
              if (trace_token)
                std::cout << "   -> REJECTED: Leading ASCII whitespace.\n";
              ptr[i] = -std::numeric_limits<float>::infinity();
              continue;
            }
          }

          // 1b. AT MOST ONE WHITESPACE-ONLY CONTINUATION PER FIELD. Once the
          // buffer already ends in whitespace, a further pure-whitespace
          // token is never productive and would let the model pad forever.
          if (!current_buffer.empty() && endsWithTokenizerSpace(current_buffer) &&
              isPureTokenizerSpace(token_str)) {
            if (trace_token)
              std::cout << "   -> REJECTED: whitespace padding onto "
                           "already-whitespace-terminated buffer.\n";
            ptr[i] = -std::numeric_limits<float>::infinity();
            continue;
          }

          // 2. STRING VALIDATION
          if (isString) {
            // 2a. STRIP TOKENIZER SPACES
            size_t start_idx = 0;
            while (start_idx < sv.size()) {
              unsigned char c = sv[start_idx];
              if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
                start_idx++;
              } else if (start_idx + 2 < sv.size() &&
                         sv.substr(start_idx, 3) == "\xE2\x96\x81") {
                start_idx += 3;  // Strip SentencePiece ' '
              } else if (c == 0xC4 && start_idx + 1 < sv.size() &&
                         static_cast<unsigned char>(sv[start_idx + 1]) ==
                             0xA0) {
                start_idx += 2;  // Strip BPE 'Ġ'
              } else {
                break;
              }
            }
            std::string_view clean_sv = sv.substr(start_idx);

            if (clean_sv.empty()) {
              // Whitespace/tokenizer-space padding is only valid as the
              // very first token of the field.
              if (current_buffer.empty()) continue;
              ptr[i] = -std::numeric_limits<float>::infinity();
              continue;
            }

            if (clean_sv.front() != '"') {
              ptr[i] = -std::numeric_limits<float>::infinity();
              continue;
            }

            bool in_escape = false;
            bool is_closed = false;
            size_t close_pos = std::string_view::npos;
            for (size_t j = 1; j < clean_sv.size(); ++j) {
              if (in_escape)
                in_escape = false;
              else if (clean_sv[j] == '\\')
                in_escape = true;
              else if (clean_sv[j] == '"') {
                is_closed = true;
                close_pos = j;
                break;
              }
            }

            // 2b. ORIGINAL PARTIAL VALIDATION (Strips opening quote, checks raw
            // payload)
            if (!is_closed) {
              std::string payload(clean_sv.substr(1));
              // isComplete=false: this string is still open, so constraints
              // like IN are checked as a prefix match rather than requiring
              // the payload to already equal one of the allowed values.
              if (rt.validatePartial(payload, /*isComplete=*/false) ==
                  ValidationStatus::Invalid) {
                ptr[i] = -std::numeric_limits<float>::infinity();
              }
              continue;
            }

            // 2c. ORIGINAL CLOSED VALIDATION (Strips both quotes, checks raw
            // payload)
            std::string payload(clean_sv.substr(1, close_pos - 1));
            ValidationStatus status = rt.validatePartial(payload);

            if (status != ValidationStatus::Valid) {
              ptr[i] = -std::numeric_limits<float>::infinity();
              continue;
            }

            std::string_view trailing = clean_sv.substr(close_pos + 1);
            size_t t_first = trailing.find_first_not_of(" \t\r\n");
            if (t_first != std::string_view::npos) {
              char exit_c = trailing[t_first];
              if (exit_c != ',' && exit_c != '}' && exit_c != '\n') {
                ptr[i] = -std::numeric_limits<float>::infinity();
              }
            }
            continue;
          }

          // 3. NON-STRING VALIDATION (Integer, Number, Boolean)
          if (sv.find('"') != std::string_view::npos) {
            if (trace_token)
              std::cout
                  << "   -> REJECTED: Contains quote in non-string field.\n";
            ptr[i] = -std::numeric_limits<float>::infinity();
            continue;
          }

          size_t exit_pos = sv.find_first_of(",}\n");
          bool isExit = (exit_pos != std::string_view::npos);
          std::string_view val = isExit ? sv.substr(0, exit_pos) : sv;

          // =========================================================
          // STRIP TOKENIZER SPACES (With Clang warning fixed)
          // =========================================================
          size_t start_idx = 0;
          while (start_idx < val.size()) {
            unsigned char c = val[start_idx];
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
              start_idx++;
            } else if (start_idx + 2 < val.size() &&
                       val.substr(start_idx, 3) == "\xE2\x96\x81") {
              start_idx += 3;  // Strip SentencePiece ' '
            } else if (c == 0xC4 && start_idx + 1 < val.size() &&
                       static_cast<unsigned char>(val[start_idx + 1]) == 0xA0) {
              start_idx += 2;  // Strip BPE 'Ġ'
            } else {
              break;
            }
          }
          val = val.substr(start_idx);

          // Strip trailing ASCII whitespace. hadTrailingWhitespace is only
          // safe to allow once the content before it is already complete
          // (canExit=true) -- e.g. "160" then a space before the comma --
          // checked below once canExit is known.
          size_t val_end = val.find_last_not_of(" \t\r\n");
          bool hadTrailingWhitespace = false;
          if (val_end != std::string_view::npos) {
            hadTrailingWhitespace = (val_end + 1 < val.size());
            val = val.substr(0, val_end + 1);
          }

          if (trace_token) {
            std::cout << "   -> Value extracted for structural check: '" << val
                      << "' (Length: " << val.size() << ")\n";
          }

          bool structurallyValid = false;
          bool canExit = false;

          if (val.empty()) {
            // Pure whitespace/tokenizer block. Only tolerated as the very
            // first token for this field.
            structurallyValid = current_buffer.empty();
            canExit = false;
            if (trace_token)
              std::cout << "   -> structural check: "
                        << (structurallyValid ? "Passed" : "REJECTED")
                        << " (Pure Tokenizer Whitespace/Prefix)\n";
          } else if (isInteger) {
            structurallyValid = isValidPartialJsonInteger(val, canExit);
            if (trace_token)
              std::cout << "   -> isInteger check: structurallyValid="
                        << structurallyValid << ", canExit=" << canExit << "\n";
          } else if (isNumber) {
            structurallyValid = isValidPartialJsonNumber(val, canExit);
          } else if (isBool) {
            structurallyValid = isValidPartialJsonBoolean(val, canExit);
          } else {
            structurallyValid = true;
            canExit = true;
          }

          if (hadTrailingWhitespace && !canExit) {
            if (trace_token)
              std::cout << "   -> REJECTED: trailing whitespace padding an "
                           "incomplete value.\n";
            structurallyValid = false;
          }

          if (!structurallyValid || (isExit && !canExit)) {
            if (trace_token)
              std::cout << "   -> REJECTED: Structural validation failed or "
                           "premature exit.\n";
            ptr[i] = -std::numeric_limits<float>::infinity();
            continue;
          }

          // Semantic validation. Even mid-number, route through
          // validatePartial(isComplete=false) so an already-out-of-range
          // prefix (e.g. "160" against `<= 80.0`) gets pruned early instead
          // of waiting for exit, by which point it's irrevocably committed.
          //
          // isComplete isn't just isExit: once hadTrailingWhitespace is
          // true, canExit was already guaranteed true above, so no more
          // digits can legally follow -- the value is locked in even
          // without a delimiter yet, and must be checked at full strictness.
          ValidationStatus status;
          bool isComplete = isExit || hadTrailingWhitespace;
          if (!val.empty() && (isInteger || isNumber)) {
            status = rt.validatePartial(val, /*isComplete=*/isComplete);
            if (trace_token)
              std::cout << "   -> semantic check: rt.validatePartial(isComplete="
                        << isComplete << ") returned enum code "
                        << static_cast<int>(status) << "\n";
          } else {
            status = rt.validatePartial(std::string(val));
            if (trace_token)
              std::cout << "   -> semantic check: rt.validatePartial() "
                           "returned enum code "
                        << static_cast<int>(status) << "\n";
          }

          if (status == ValidationStatus::Invalid ||
              (isExit && status != ValidationStatus::Valid)) {
            if (trace_token)
              std::cout << "   -> REJECTED: Semantic check failed.\n";
            ptr[i] = -std::numeric_limits<float>::infinity();
            continue;
          }

          if (trace_token)
            std::cout << "   -> ACCEPTED: Token survived the mask.\n";
          surviving_tokens++;
        }

        if (verbose) {
          std::cout << "[C++ Mask] FINISHED. " << surviving_tokens
                    << " tokens survived.\n";
          std::cout << "==================================================\n";
        }
      },
      "Masks logits across the entire vocabulary using strict C++ invariants.",
      py::arg("rt"), py::arg("logits"), py::arg("vocab"),
      py::arg("current_buffer"), py::arg("verbose") = false);
}