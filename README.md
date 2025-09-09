# toycpp: A toy C++ compiler

I'm interested in implementing a compiler for some subset of the C++ language, as a learning exercise, inspired by [Tsoding's recreational programming](<https://www.youtube.com/@TsodingDaily/videos?view=0&sort=dd&shelf_id=1>).

The compiler's goal is to produce X86_64 assembly.

## Goals

Potentially in order, probably incomplete:

- [ ] Preprocessor support (`#ifdef`, `#include`)
- [ ] Lexing and parsing into an AST
- [ ] Compiling an entry point's `return`
- [ ] Arithmetic - integer addition, subtraction, multiplication and division
- [ ] Function definition and calling

## Non-goals

This is a project meant for learning, so a few things that would otherwise be priorities fly out the window:

- **Optimization**: I'll write things as sloppily and clearly as I can - I'll only optimize once things become too painful.
- **Innovation/Originality**: At some point, I might cheat and copy things to help me move forward.
- **Linking and Assembling**: I don't want to replace ld/[lld](<https://lld.llvm.org/>)/[mold](<https://github.com/rui314/mold>) or GNU Assembler/Nasm/fasm
