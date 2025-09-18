# toycpp: A toy C++ compiler

I'm interested in implementing a compiler for some subset of the C++ language, as a learning exercise, inspired by [Tsoding's recreational programming](<https://www.youtube.com/@TsodingDaily/videos?view=0&sort=dd&shelf_id=1>).

The compiler's goal is to produce X86_64 assembly.

## Inspirations/resources

- [Tsoding's recreational programming series](<https://www.youtube.com/@TsodingDaily/videos?view=0&sort=dd&shelf_id=1>)
- [Bisqwit's "Creating a compiler" series](<https://www.youtube.com/watch?v=KwpcOYKfXZc&list=PLzLzYGEbdY5n9ITKUqOuRjXkRU5tMW2Sd>)
- [The FASM 1.73 manual](<https://flatassembler.net/docs.php?article=manual>)
- [en.cppreference.com](<https://en.cppreference.com/w/cpp.html>), which I access through [DevDocs](<https://devdocs.io/>)
- [The combined Intel IA-32/64-bit Software Developer's Manual](<https://cdrdv2-public.intel.com/858440/325462-088-sdm-vol-1-2abcd-3abcd-4.pdf>)

## Development and Usage

1. Install [`fasm`](<https://flatassembler.net/>), [`git`](<https://git-scm.com/downloads>), a C++ compiler supporting C++17 and GNU Make:
   ```bash
   sudo apt install git fasm gcc-c++ make
   ```
2. Clone the project and compile toycpp using `make`:
   ```bash
   git clone https://github.com/PracticallyNothing/toycpp
   cd toycpp
   make -j4
   ```
3. Run `toycpp` on a C++ file - this will produce a file called `executable` in the current directory.
   ```bash
   ./toycpp test/add.cpp
   ```
4. Run `executable` - voila!
   ```bash
   ./executable
   ```

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
