#include "ast.hpp"

#include "color.hpp"
#include "lex.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

namespace ast {
using std::cerr, std::endl, std::string;

Expression *parseExpression(lex::Lexer &lexer) {
  lex::Token t = lexer.nextToken();

  switch (t.type) {
  case lex::NumberLiteral: {
    lex::Token next = lexer.peek();

    switch (next.type) {
    case lex::Semicolon:
      return new Expression{
          .type = Expr_IntConstant,
          .integer = (int) t,
      };
    default:
      cerr << color::boldred("ERROR") << ": Unexpected token " << next
           << " while parsing expression!" << endl;
      abort();
    };

  }; break;

  case lex::Identifier: {
    lex::Token next = lexer.peek();
    switch (next.type) {
    case lex::Plus:
      lexer.eatToken(lex::Plus);
      return new Expression{
          .type = Expr_BinaryOp,
          .lhs =
              new Expression{
                  .type = Expr_VarAccess,
                  .identifier = string(next.span),
              },
          .rhs = parseExpression(lexer),
      };
    case lex::Semicolon:
      return new Expression{
          .type = Expr_VarAccess,
          .identifier = string(next.span),
      };
    default:
      cerr << color::boldred("ERROR") << ": Unexpected token " << next
           << " while parsing expression!" << endl;
      abort();
    }
  } break;

  default:
    cerr << color::boldred("ERROR") << ": Unexpected token " << t
         << " while parsing expression!" << endl;
    abort();
  }
}
} // namespace ast
