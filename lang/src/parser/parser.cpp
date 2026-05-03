#include "parser.hpp"

using namespace invariants::parser;

Parser::Parser(const std::vector<lexer::Token>& tokens) : tokens(tokens) {}