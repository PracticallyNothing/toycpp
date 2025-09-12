#pragma once

#include <string>
#include "ast.hpp"

namespace compile {
  using std::string;

  string compileProgram(ast::Program);
}
