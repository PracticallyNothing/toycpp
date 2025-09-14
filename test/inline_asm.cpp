void print_source_code() {
  asm("mov rax, 1\n"
      "mov rdi, 1\n"
      "mov rsi, inline_asm\n"
      "mov rdx, 268\n"
      "syscall");
  return;

  asm("inline_asm:\n"
      "  file \"test/inline_asm.cpp\"");
}

int main() {
  print_source_code();
  return 17;
}
