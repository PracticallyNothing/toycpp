#pragma once

#include <array>
#include <cassert>
#include <optional>
#include <string>
#include <string_view>

namespace lex {
enum TokenType {
  Invalid,
  Eof,

  NumberLiteral,    //
  CharLiteral,      //
  StringLiteral,    //
  RawStringLiteral, //

  Identifier, // ...

  Minus, // -
  Plus,  // +
  Slash, // /
  Comma, // ,

  Equal,       // =
  LessThan,    // <
  GreaterThan, // >

  Not,       // !
  Dot,       // .
  Star,      // *
  Ampersand, // &
  BitwiseOr, // |

  Colon,     // :
  Semicolon, // ;

  LParen,   // (
  RParen,   // )
  LSquare,  // [
  RSquare,  // ]
  LBracket, // {
  RBracket, // }

  LessThanOrEqual,    // <=
  GreaterThanOrEqual, // >=
  EqualEqual,         // ==
  NotEqual,           // !=
  Increment,          // ++
  Decrement,          // --

  Arrow, // ->

  LogicalAnd, // &&
  LogicalOr,  // ||

  BasicType,     // int, char, float, double, bool, void
  IntModifier,   // "unsigned" | "short" | "long"
  ValueModifier, // "const" | "volatile" | "constexpr"
  Keyword,

  AnyToken, // Any token - default value for Lexer::nextToken().
};

const std::array BasicTypes = {
    "int",  "char", "void", "float", "double", "bool",
    "auto", // TODO: Support auto
};

const std::array IntModifiers = {
    "unsigned",
    "short",
    "long",
};

const std::array ValueModifiers = {
    "const",
    "volatile",
    "constexpr",
};

const std::array Keywords = {
    "true",   "false",

    "if",     "else",     "switch",  "case",      "default:",

    "for",    "while",    "do",      "continue",  "break",    "return",

    "struct", "class",    "typedef", "namespace", "using",

    "const",  "volatile", "auto",
};

struct Token {
  Token() : type(Invalid), span() {}
  Token(TokenType type, std::string_view span) : type(type), span(span) {}

  TokenType type;
  std::string_view span;

  inline operator int() {
    assert(type == NumberLiteral);
    return std::stoi(std::string(span));
  }
};

class Lexer {
public:
  Lexer(const std::string &src)
      : _src(src.c_str()), _length(src.length()), _head(src.c_str()) {};

  Lexer(const char *src, size_t length) : _src(src), _length(length), _head(src) {};

  void eatToken(TokenType expected);
  Token nextToken(TokenType expected = AnyToken);

  /// Look at the next token without actually advancing to it.
  Token peek();

private:
  // Look at the current character.
  inline char curr() const { return *_head; }

  // Look at the next character.
  inline char next() const { return *(_head + 1); }

  void _skipWhitespace();

  bool _isEOF();

  /// Consume characters until a separator character is found.
  std::string_view _eatNextWord();

  /// The source code that this lexer is parsing.
  const char *const _src;

  /// The length of the source code string.
  const size_t _length;

  /// The position where the Lexer is currently located.
  const char *_head;
};

} // namespace lex

extern std::ostream &operator<<(std::ostream &o, lex::Token token);
extern std::ostream &operator<<(std::ostream &o, lex::TokenType type);
