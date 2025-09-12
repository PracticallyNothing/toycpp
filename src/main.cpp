#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "ast.hpp"
#include "color.hpp"
#include "compile.hpp"
#include "lex.hpp"

using std::cerr, std::cout, std::endl, std::ifstream, std::string;

std::ostream &operator<<(std::ostream &o, ast::Type type) {
  o << "Type(kind: " << type.kind << ", name: " << type.name << ")";
  return o;
}

std::ostream &operator<<(std::ostream &o, ast::FunctionDefinition funcDef) {
  o << "FunctionDefinition(returnType: " << funcDef.returnType << ", name: <"
    << funcDef.name << ">, arguments: [], body: [])";
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
  ast::Program program;

  for (lex::Token t = lexer.nextToken(); t.type != lex::Eof;
       t = lexer.nextToken()) {
    switch (t.type) {
    case lex::BasicType: {
      // This is a function definition.
      lex::Token returnType = t;
      lex::Token functionName = lexer.nextToken(lex::Identifier);

      lexer.eatToken(lex::LParen);
      // TODO: Handle arguments.
      lexer.eatToken(lex::RParen);

      // Begin consuming function body.
      lexer.eatToken(lex::LBracket);
      std::vector<ast::Statement> body;

      while (t.type != lex::Eof && t.type != lex::RBracket) {
        t = lexer.nextToken();

        switch (t.type) {
        case lex::RBracket:
          break;
        case lex::Keyword: {
          if (t.span == "return") {
            ast::ReturnStatement returnStmt;

            lex::Token returnValue = lexer.nextToken(lex::NumberLiteral);
            returnStmt.returnValue = std::stoi(std::string(returnValue.span));

            lexer.eatToken(lex::Semicolon);

            body.push_back(returnStmt);
          }
        } break;
        default:
          cerr << color::boldred("ERROR") << ": Unexpected token " << t << "!"
               << endl;
          exit(1);
        }
      }

      ast::FunctionDefinition funcDef;
      funcDef.returnType = ast::Type::FromBasicType(returnType);
      funcDef.name = functionName.span;
      funcDef.body = body;

      program.funcDefs.push_back(funcDef);
    } break;

    default:
      cerr << color::boldred("ERROR") << ": Unexpected token '" << t << "'!"
           << endl;
      exit(-1);
    }
  }

  cout << "Got a program with " << program.funcDefs.size() << " functions!"
       << endl;

  std::string assembly = compile::compileProgram(program);

  cout << "--------------------------" << endl
       << assembly << endl
       << "--------------------------" << endl;

  std::ofstream outputFile(std::string(argv[1]) + ".asm");
  outputFile << assembly;

  return 0;
}
