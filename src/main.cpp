#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>

#include "color.hpp"
#include "lex.hpp"

using std::cerr, std::cout, std::endl, std::ifstream, std::string;

std::ostream &operator<<(std::ostream &o, lex::Token t) {
  o << "Token(type: " << t.type << ", span: <" << t.span << ">)";
  return o;
}

// Thank you,
// https://stackoverflow.com/a/116220
std::string slurp(std::ifstream &in) {
  std::ostringstream sstr;
  sstr << in.rdbuf();
  return sstr.str();
}

int main(int argc, const char **argv) {
  if (argc != 2) {
    cerr << "ERROR: Not enough/too many arguments!" << endl;
    exit(-1);
  }

  ifstream source_file(argv[1]);
  if (!source_file.is_open() || !source_file.good()) {
    cerr << "ERROR: Failed to read or open ''" << argv[1] << "'!";
  }
  string sourceCode = slurp(source_file);

  lex::Lexer lexer(sourceCode);

  for (lex::Token t = lexer.nextToken(); t.type != lex::Eof;
       t = lexer.nextToken()) {
    cout << t << endl;

    switch (t.type) {
    case lex::BasicType: {
      // This is a function definition.
      lex::Token returnType = t;
      lex::Token functionName = lexer.nextToken(lex::Identifier);

      lexer.eatToken(lex::LParen);
      // TODO: Handle arguments.
      lexer.eatToken(lex::RParen);

      lexer.eatToken(lex::LBracket);
      // TODO: Function body.
      lexer.eatToken(lex::RBracket);

      cout << "Got a function! -> " << returnType.span << " "
           << functionName.span << "() {}" << endl;
    } break;

    default:
      cerr << color::boldred("ERROR") << ": Unexpected token '" << t << "'!"
           << endl;
      exit(-1);
    }
  }

  return 0;
}
