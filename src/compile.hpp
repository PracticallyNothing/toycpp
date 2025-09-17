#pragma once

#include "ast.hpp"

#include <cstddef>
#include <map>
#include <string>

namespace compile {
using std::string, std::map;

struct VariableInfo {
  size_t stackPos;
  size_t size;
};

struct Context {
  size_t currStackPos = 0;
  std::map<string, VariableInfo> variables;
};

string compileProgram(ast::Program);
} // namespace compile
