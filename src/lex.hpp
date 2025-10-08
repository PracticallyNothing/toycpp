#pragma once

#include "utils.hpp"

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

  AnyToken, // Any token - default value for Lexer::nextToken().
};

struct Token {
  Token() : type(Invalid), span() {}
  Token(TokenType type, std::string_view span, Location location)
      : type(type), span(span), location(location) {}

  TokenType type;
  std::string_view span;
  Location location;
};

class Lexer {
public:
  Lexer(const std::string filename, const std::string &src)
      : _filename(filename), _src(src.c_str()), _length(src.length()),
        _head(src.c_str()), lineStart(_src - 1) {}

  Lexer(const std::string filename, const char *src, size_t length)
      : _filename(filename), _src(src), _length(length), _head(src),
        lineStart(_src - 1) {}

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

  const char *findLineEnd() const;

  /// Consume characters until a separator character is found.
  std::string_view _eatNextWord();

  const std::string _filename;

  /// The source code that this lexer is parsing.
  const char *const _src;

  /// The length of the source code string.
  const size_t _length;

  /// The position where the Lexer is currently located.
  const char *_head;

  /// The current line that the lexer is located on.
  unsigned currLine = 1;

  const char *lineStart;
};

std::ostream &operator<<(std::ostream &o, lex::Token token);
std::ostream &operator<<(std::ostream &o, lex::TokenType type);

} // namespace lex
