#pragma once

#include "lex.hpp"
#include <cassert>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace ast {
using std::string, std::vector, std::optional, std::variant;

enum TypeKind { Void, Char, Int, Float, Double, Bool, Auto, Class };

struct Type {
  static Type FromBasicType(lex::Token t) {
    assert(t.type == lex::BasicType);
    Type result;

    result.name = t.span;

    if (t.span == "void") {
      result.kind = Void;
    } else if (t.span == "int") {
      result.kind = Int;
    } else if (t.span == "float") {
      result.kind = Float;
    } else if (t.span == "double") {
      result.kind = Double;
    } else if (t.span == "bool") {
      result.kind = Bool;
    } else if (t.span == "auto") {
      result.kind = Auto;
    } else {
      result.kind = Class;
    }

    return result;
  }

  TypeKind kind;
  string name;
  // TODO: Support const + volatile.

  // TODO: Support lvalue references.
  // TODO: Support pointers.

  // TODO: Support rvalue references.
};

struct ReturnStatement {
  optional<int> returnValue;
};

struct FuncCallStatement {
  string functionName;
};

struct InlineAssemblyStatement {
  string content;
};

using Statement =
    variant<ReturnStatement, FuncCallStatement, InlineAssemblyStatement>;

struct FuncParameter {
  Type type;
  string name;
  // TODO: Support initializer (a.k.a default value).
};

struct FunctionDefinition {
  Type returnType;
  string name;
  vector<FuncParameter> parameters;
  vector<Statement> body;
};

struct Program {
  vector<FunctionDefinition> funcDefs;
};

} // namespace ast
