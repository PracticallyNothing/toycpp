#include "compile.hpp"

#include "ast.hpp"
#include "utils.hpp"

#include <cassert>
#include <sstream>

namespace compile {
using std::stringstream;

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

  for (const auto &funcDef : program.funcDefs) {
    result << funcDef.name << ":\n"
           << "  push rbp\n"
           << "  mov rbp, rsp\n\n";

    for (const auto &statement : funcDef.body) {
      std::visit(Overloaded{
                     [&result](ast::FuncCallStatement funcCall) {
                       result << "  call " << funcCall.functionName << "\n";
                     },
                     [&result](ast::InlineAssemblyStatement inlineAsm) {
                       result << inlineAsm.content << "\n";
                     },
                     [&result, &funcDef](ast::ReturnStatement ret) {
                       if (ret.returnValue.has_value()) {
                         result << "  mov rax, " << ret.returnValue.value().integer
                                << "\n";
                       }
                       result << "  jmp " << funcDef.name << "__return\n";
                     },
                     [](auto node) {},
                 },
                 statement);
    }

    result << funcDef.name << "__return:\n"
           << "  pop rbp\n"
           << "  ret\n\n";
  }

  return result.str();
}
} // namespace compile
