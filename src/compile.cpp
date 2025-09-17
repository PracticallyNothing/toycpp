#include "compile.hpp"

#include "ast.hpp"
#include "utils.hpp"

#include <cassert>
#include <cstdlib>
#include <iomanip>
#include <ostream>
#include <sstream>
#include <stack>

namespace compile {
using std::stringstream;

std::ostream &operator<<(std::ostream &os, ast::Expression expr) {
  switch (expr.type) {
  case ast::Expr_IntConstant   : os << expr.integer; break;
  case ast::Expr_StringConstant: os << std::quoted(expr.string); break;
  case ast::Expr_VarAccess     : os << expr.identifier; break;
  case ast::Expr_UnaryOp       : os << expr.unaryOpType << " " << *expr.lhs; break;
  case ast::Expr_BinaryOp:
    os << *expr.lhs;
    switch (expr.binOpType) {
    case ast::BinOp_Add               : os << " + "; break;
    case ast::BinOp_Sub               : os << " - "; break;
    case ast::BinOp_Divide            : os << " / "; break;
    case ast::BinOp_Mult              : os << " * "; break;
    case ast::BinOp_Modulo            : os << " % "; break;
    case ast::BinOp_Equal             : os << " == "; break;
    case ast::BinOp_NotEqual          : os << " != "; break;
    case ast::BinOp_LessThan          : os << " < "; break;
    case ast::BinOp_GreaterThan       : os << " > "; break;
    case ast::BinOp_LessThanOrEqual   : os << " <= "; break;
    case ast::BinOp_GreaterThanOrEqual: os << " >="; break;
    }
    os << *expr.rhs;
    break;
  }
  return os;
}

enum class reg {
  eax,
  ebx,
  esi,
  edi,

  rax,
  rbx,
  rsi,
  rdi,
};

static std::ostream &operator<<(std::ostream &os, reg reg) {
  switch (reg) {
  case reg::eax: os << "eax"; break;
  case reg::ebx: os << "ebx"; break;
  case reg::esi: os << "esi"; break;
  case reg::edi: os << "edi"; break;

  case reg::rax: os << "rax"; break;
  case reg::rbx: os << "rbx"; break;
  case reg::rsi: os << "rsi"; break;
  case reg::rdi: os << "rdi"; break;
  }
  return os;
}

/// Produce an assembly instruction that sets `destination` to the value (at) `source`.
template<typename Dest, typename Src>
string set(Dest destination, Src source);

/// Produce an assembly instruction that essentially does `destination += source`.
template<typename Dest, typename Src>
string addTo(Dest destination, Src source);

template<>
string addTo(reg dest, int src) {
  stringstream ss;
  ss << "  add " << dest << ", " << src << "\n";
  return ss.str();
}

template<>
string addTo(reg dest, reg src) {
  stringstream ss;
  ss << "  add " << dest << ", " << src << "\n";
  return ss.str();
}

template<>
string addTo(reg dest, VariableInfo src) {
  stringstream ss;
  ss << "  add " << dest << ", [rsp-" << src.size + src.stackPos << "]\n";
  return ss.str();
}

template<>
string set(reg dest, int src) {
  stringstream ss;
  ss << "  mov " << dest << ", " << src << "\n";
  return ss.str();
}

template<>
string set(reg dest, reg src) {
  stringstream ss;
  ss << "  mov " << dest << ", " << src << "\n";
  return ss.str();
}

template<>
string set(reg dest, VariableInfo src) {
  stringstream ss;
  ss << "  mov " << dest << ", [rsp-" << src.size + src.stackPos << "]" << "\n";
  return ss.str();
}

template<>
string set(VariableInfo dest, int src) {
  stringstream ss;
  ss << "  mov dword [rsp-" << dest.size + dest.stackPos << "], " << src << "\n";
  return ss.str();
}

template<>
string set(VariableInfo dest, reg src) {
  stringstream ss;
  ss << "  mov dword [rsp-" << dest.size + dest.stackPos << "], " << src << "\n";
  return ss.str();
}

string compileProgram(ast::Program program) {
  assert(!program.funcDefs.empty());

  stringstream result;

  result << "format ELF64 executable\n\n";
  result << "_start:\n"
         << "  ;; Initialize globals\n"
         << "  ;; ...\n\n"
         << "  ;; Call main\n"
         << "  call main\n\n"
         << "  ;; Exit with status code = result from main.\n"
         << "  mov rdi, rax                ; return code: whatever main returned\n"
         << "  mov rax, 60                 ; sys_exit(fd)\n"
         << "  syscall\n\n";

  Context currContext;

  for (const auto &funcDef : program.funcDefs) {
    result << funcDef.name << ":\n"
           << "  push rbp\n"
           << "  mov rbp, rsp\n\n";

    for (const auto &statement : funcDef.body) {
      std::visit(
          Overloaded{
              [&result, &currContext](ast::VarDefStmt def) {
                assert(def.type.kind != ast::Void);

                size_t totalSize = 0;

                for (const auto &name : def.names) {
                  VariableInfo varInfo{
                      .stackPos = currContext.currStackPos,
                      .size = 4,
                  };

                  // Allocate the variable.
                  currContext.variables[name] = varInfo;

                  // Record how much space it took up.
                  currContext.currStackPos += varInfo.size;
                  totalSize += varInfo.size;
                }

                result << "  sub rsp, " << totalSize << "   ; ";

                for (size_t i = 0; i < def.names.size(); i++) {
                  if (i > 0) result << ", ";
                  result << def.names[i];
                }
                result << "\n";
              },
              [&result, &currContext](ast::VarAssignStmt assignment) {
                const auto &expr = assignment.expression;
                const auto &varInfo = currContext.variables.at(assignment.varName);

                result << "  ;; " << assignment.varName << " = "
                       << assignment.expression << ";\n";

                switch (expr.type) {
                case ast::Expr_IntConstant:
                  result << set(varInfo, expr.integer) << "\n";
                  break;
                case ast::Expr_BinaryOp: {
                  auto lhs = assignment.expression.lhs;
                  auto rhs = assignment.expression.rhs;

                  // FIXME(Mario, 2025-09-17):
                  //   Support something other than addition...
                  assert(expr.binOpType == ast::BinOp_Add);

                  if (lhs->type == ast::Expr_IntConstant)
                    result << set(reg::eax, lhs->integer);
                  else
                    result << set(reg::eax, currContext.variables.at(lhs->identifier));

                  if (lhs->type == ast::Expr_IntConstant)
                    result << addTo(reg::eax, rhs->integer);
                  else
                    result << addTo(reg::eax,
                                    currContext.variables.at(rhs->identifier));

                  result << set(varInfo, reg::eax) << "\n";

                } break;

                  // default: abort();
                }
              },
              [&result](ast::FuncCallStatement funcCall) {
                result << "  call " << funcCall.functionName << "\n";
              },
              [&result](ast::InlineAssemblyStatement inlineAsm) {
                result << inlineAsm.content << "\n";
              },
              [&result, &currContext, &funcDef](ast::ReturnStatement ret) {
                if (ret.returnValue.has_value()) {
                  auto expr = ret.returnValue.value();
                  switch (expr.type) {
                  case ast::Expr_IntConstant:
                    result << set(reg::rax, expr.integer);
                    break;
                  case ast::Expr_VarAccess: {
                    auto varInfo = currContext.variables.at(expr.identifier);

                    result << "  ;; return " << expr.identifier << ";\n";
                    result << "  mov rax, [rsp-" << varInfo.stackPos + varInfo.size
                           << "]" << "\n";
                  } break;
                  default: abort();
                  }
                }
                result << "  jmp " << funcDef.name << "__return\n";
              },
              [](auto node) {},
          },
          statement);
    }

    result << funcDef.name << "__return:\n"
           << "  add rsp, " << currContext.currStackPos << "\n"
           << "  pop rbp\n"
           << "  ret\n\n";
  }

  return result.str();
}
} // namespace compile
