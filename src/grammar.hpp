#pragma once

#include "lex.hpp"

#include <optional>
#include <string>
#include <vector>

namespace grammar {
using std::string, std::vector, std::optional;

struct Grammar;
struct Node {
  string name;
  vector<Node> children;
  bool isTerminal = false;
};

extern Grammar *parseGrammarFile(const string filename);
extern Node parse(const Grammar *grammar, lex::Lexer &lexer);
extern void printNodeTree(const Node &root);
} // namespace grammar
