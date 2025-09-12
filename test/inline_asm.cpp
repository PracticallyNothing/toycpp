int print() {
  asm(
    "mov rax, 1\n"
    "mov rdi, 1\n"
    "mov rsi, inline_asm\n"
    "mov rdx, 100\n"
    "syscall"
  );
}

int main() {
  print();
  return 17;

  asm(
    "inline_asm:\n"
    "  file \"inline_asm.cpp\""
  );
}
