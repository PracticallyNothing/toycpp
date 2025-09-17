#include "ast.hpp"
#include "color.hpp"
#include "compile.hpp"
#include "lex.hpp"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

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

  for (lex::Token t = lexer.nextToken(); t.type != lex::Eof; t = lexer.nextToken()) {
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
        case lex::RBracket: break;

        case lex::BasicType: {
          auto varType = ast::Type::FromBasicType(t);
          auto varName = lexer.nextToken(lex::Identifier);
          lexer.eatToken(lex::Semicolon);

          body.push_back(ast::VarDefStmt{
              .type = varType,
              .names = {string(varName.span)},
          });
        } break;

        case lex::Identifier: {
          if (t.span == "asm") {
            lexer.eatToken(lex::LParen);

            std::stringstream assemblyText;
            for (lex::Token asmToken = lexer.nextToken();
                 asmToken.type != lex::Eof && asmToken.type != lex::RParen;
                 asmToken = lexer.nextToken()) {
              if (asmToken.type != lex::StringLiteral) {
                cerr << color::boldred("ERROR") << ": Expected StringLiteral, but got "
                     << asmToken << "!" << endl;
                exit(-1);
              }

              for (size_t i = 1; i < asmToken.span.length(); i++) {
                char c = asmToken.span[i];

                if (c == '\\') {
                  i++;
                  switch (asmToken.span[i]) {
                  case 'n' : c = '\n'; break;
                  case 'r' : c = '\r'; break;
                  case '\\': c = '\\'; break;
                  default  : c = asmToken.span[i]; break;
                  }
                }

                assemblyText << c;
              }
            }
            lexer.eatToken(lex::Semicolon);

            ast::InlineAssemblyStatement inlineAsm;
            inlineAsm.content = assemblyText.str();

            body.push_back(inlineAsm);
          } else {
            lex::Token nextToken = lexer.nextToken();

            if (nextToken.type == lex::LParen) {
              lexer.eatToken(lex::RParen);
              lexer.eatToken(lex::Semicolon);

              ast::FuncCallStatement funcCall{.functionName = string(t.span)};
              body.push_back(funcCall);
            } else if (nextToken.type == lex::Equal) {
              ast::VarAssignStmt assignment{.varName = string(t.span),
                                            .expression = *ast::parseExpression(lexer)};
              lexer.eatToken(lex::Semicolon);
              body.push_back(assignment);
            } else {
              cerr << color::boldred("ERROR") << ": Expected '(' or '=', but got "
                   << nextToken << "!" << endl;
              exit(1);
            }
          }
        } break;
        case lex::Keyword: {
          if (t.span == "return") {
            ast::ReturnStatement returnStmt;

            lex::Token token = lexer.peek();
            if (token.type != lex::Semicolon) {
              returnStmt.returnValue = *ast::parseExpression(lexer);
            }
            lexer.eatToken(lex::Semicolon);

            body.push_back(returnStmt);
          }
        } break;
        default:
          cerr << color::boldred("ERROR") << ": Unexpected token " << t << "!" << endl;
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
      cerr << color::boldred("ERROR") << ": Unexpected token '" << t << "'!" << endl;
      exit(-1);
    }
  }

  std::string assembly = compile::compileProgram(program);

  cout << "--------------------------" << endl
       << assembly << endl
       << "--------------------------" << endl;

  {
    std::ofstream outputFile("/tmp/toycpp_output.asm");
    outputFile << assembly;
    outputFile.close();
  }

  system("fasm /tmp/toycpp_output.asm executable");

  return 0;
}
