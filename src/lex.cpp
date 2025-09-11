#include "lex.hpp"
#include "color.hpp"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <ostream>

using std::cerr, std::cout, std::endl;

namespace lex {

template <typename T, typename U, size_t N>
bool arrayContains(std::array<T, N> arr, U value) {
  for (const auto &v : arr) {
    if (v == value)
      return true;
  }

  return false;
}

Token Lexer::nextToken(TokenType expected) {
  _skipWhitespace();

  Token result;

  if (_isEOF()) {
    if (expected != AnyToken && result.type != expected) {
      cerr << color::boldred("ERROR") << ": Expected token of type " << expected
           << "but got " << result.type << "!";
      exit(1);
    }
    result.type = Eof;
    return result;
  }

  char currChar = *_head;

  if (isalpha(currChar)) {
    auto nextWord = _eatNextWord();
    result.span = nextWord;

    if (arrayContains(BasicTypes, nextWord)) {
      result.type = BasicType;
    } else if (arrayContains(IntModifiers, nextWord)) {
      result.type = IntModifier;
    } else if (arrayContains(ValueModifiers, nextWord)) {
      result.type = ValueModifier;
    } else if (arrayContains(Keywords, nextWord)) {
      result.type = Keyword;
    } else {
      result.type = Identifier;
    }
  } else if (isdigit(currChar)) {
    exit(2);
  } else {
    switch (currChar) {
    case '{':
      result.type = LBracket;
      _head++;
      break;
    case '}':
      result.type = RBracket;
      _head++;
      break;
    case '(':
      result.type = LParen;
      _head++;
      break;
    case ')':
      result.type = RParen;
      _head++;
      break;
    case '[':
      result.type = LSquare;
      _head++;
      break;
    case ']':
      result.type = RSquare;
      _head++;
      break;
    case ';':
      result.type = Semicolon;
      _head++;
      break;
    }
  }

  if (expected != AnyToken && result.type != expected) {
    cerr << color::boldred("ERROR") << ": Expected token of type " << expected
         << ", but got " << result.type << "!";
    exit(1);
  }
  return result;
}

void Lexer::eatToken(TokenType expected) { Token t = nextToken(expected); }

bool Lexer::_isEOF() { return _head >= _src + _length; }

void Lexer::_skipWhitespace() {
  while (!_isEOF() && isspace(*_head)) {
    _head++;
  }
}

bool isOneOf(char c, const std::string symbols) {
  for (char s : symbols) {
    if (c == s)
      return true;
  }

  return false;
}

std::string_view Lexer::_eatNextWord() {
  assert(!isspace(*_head));

  const char *end = _head;
  while (end < _src + _length && !isspace(*end) &&
         !isOneOf(*end, "()[]{}.,:;-+/*^|&!%\'\"<>?!=^")) {
    end++;
  }

  auto s = std::string_view(_head, end - _head);
  _head = end;
  return s;
}

} // namespace lex
