#include "ast.hpp"
#include "color.hpp"
#include "grammar.hpp"
#include "lex.hpp"
#include "utils.hpp"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

using std::cerr, std::cout, std::endl, std::ifstream, std::string;

int main(int argc, const char **argv) {
  if (argc != 2) {
    cerr << "ERROR: Not enough/too many arguments!" << endl;
    exit(-1);
  }

  ifstream source_file(argv[1]);
  if (!source_file.is_open() || !source_file.good()) {
    cerr << "ERROR: Failed to read or open ''" << argv[1] << "'!";
    exit(1);
  }

  string sourceCode = slurp(source_file);
  lex::Lexer lexer(sourceCode);
  grammar::Grammar *grammar = grammar::parseGrammarFile("grammar.rule");

  grammar::Node rootNode = grammar::parse(grammar, lexer);
  grammar::printNodeTree(rootNode);

  return 0;
}
