#include "compile.hpp"
#include "ast.hpp"
#include <cassert>
#include <sstream>
#include <type_traits>

namespace compile {
using std::stringstream;

string compileProgram(ast::Program program) {
  assert(!program.funcDefs.empty());

  stringstream result;

  result << "format ELF64 executable\n\n";
  result
      << "_start:\n"
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
      std::visit(
          [&funcDef, &result](auto &&arg) {
            using T = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<T, ast::ReturnStatement>) {
              if (arg.returnValue.has_value()) {
                result << "  mov rax, " << arg.returnValue.value() << "\n";
              }
              result << "  jmp " << funcDef.name << "__return\n";
            } else if constexpr (std::is_same_v<T, ast::FuncCallStatement>) {
              result << "  call " << arg.functionName << "\n";
            } else if constexpr (std::is_same_v<T,
                                                ast::InlineAssemblyStatement>) {
              result << arg.content << "\n";
            } else {
              abort();
            }
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
