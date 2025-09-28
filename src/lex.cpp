#include "lex.hpp"

#include "color.hpp"

#include <cassert>
#include <iostream>
#include <ostream>
#include <string_view>

using std::cerr, std::endl;

std::ostream &operator<<(std::ostream &o, lex::Token token) {
  o << "Token(type: " << token.type << ", span: <" << token.span << ">)";
  return o;
}

std::ostream &operator<<(std::ostream &o, lex::TokenType type) {
  using namespace lex;

  switch (type) {
  case Invalid           : o << "???"; break;
  case Eof               : o << "[EOF]"; break;
  case NumberLiteral     : o << "NumberLiteral"; break;
  case CharLiteral       : o << "CharLiteral"; break;
  case StringLiteral     : o << "StringLiteral"; break;
  case RawStringLiteral  : o << "RawStringLiteral"; break;
  case Identifier        : o << "Identifier"; break;
  case Not               : o << "!"; break;
  case Minus             : o << "-"; break;
  case Plus              : o << "+"; break;
  case Slash             : o << "/"; break;
  case Comma             : o << ","; break;
  case Equal             : o << "="; break;
  case LessThan          : o << "<"; break;
  case GreaterThan       : o << ">"; break;
  case Dot               : o << "."; break;
  case Star              : o << "*"; break;
  case Ampersand         : o << "&"; break;
  case BitwiseOr         : o << "|"; break;
  case Colon             : o << ":"; break;
  case Semicolon         : o << ";"; break;
  case LParen            : o << "("; break;
  case RParen            : o << ")"; break;
  case LSquare           : o << "["; break;
  case RSquare           : o << "]"; break;
  case LBracket          : o << "{"; break;
  case RBracket          : o << "}"; break;
  case LessThanOrEqual   : o << "<="; break;
  case GreaterThanOrEqual: o << ">="; break;
  case EqualEqual        : o << "=="; break;
  case NotEqual          : o << "!="; break;
  case Increment         : o << "++"; break;
  case Decrement         : o << "--"; break;
  case Arrow             : o << "->"; break;
  case LogicalAnd        : o << "&&"; break;
  case LogicalOr         : o << "||"; break;
  case BasicType         : o << "BasicType"; break;
  case IntModifier       : o << "IntModifier"; break;
  case ValueModifier     : o << "ValueModifier"; break;
  case Keyword           : o << "Keyword"; break;
  case AnyToken          : o << "[AnyToken]"; break;
  }

  return o;
}

namespace lex {

template<typename T, typename U, size_t N>
bool arrayContains(std::array<T, N> arr, U value) {
  for (const auto &v : arr) {
    if (v == value) return true;
  }

  return false;
}

Token Lexer::peek() {
  const char *oldHead = _head;

  Token result = nextToken();

  _head = oldHead;
  return result;
}

Token Lexer::nextToken(TokenType expected) {
  _skipWhitespace();

  Token result;

  if (_isEOF()) {
    if (expected != AnyToken && result.type != expected) {
      cerr << color::boldred("ERROR") << ": Expected token of type " << expected
           << " but got " << result.type << "!" << endl;
      exit(1);
    }
    result.type = Eof;
    return result;
  }

  char currChar = *_head;

  if (isalpha(currChar) || curr() == '_') {
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
    result.type = NumberLiteral;
    result.span = _eatNextWord();
  } else if (currChar == '"') {
    result.type = StringLiteral;

    const char *end = _head + 1;
    for (; end < _src + _length; end++) {
      if (*end == '\\') {
        end++;
      } else if (*end == '"') {
        break;
      }
    }

    if (*end != '\"') {
      cerr << color::boldred("ERROR") << ": Unterminated string literal!" << endl;
      exit(1);
    }

    result.span = std::string_view(_head + 1, end - _head - 1);
    _head = end + 1;
  } else if (currChar == '\'') {
    result.type = CharLiteral;

    const char *end = _head + 1;
    for (; end < _src + _length; end++) {
      if (*end == '\\') {
        end++;
      } else if (*end == '\'') {
        break;
      }
    }

    if (*end != '\"') {
      cerr << color::boldred("ERROR") << ": Unterminated character literal!" << endl;
      exit(1);
    }

    result.span = std::string_view(_head + 1, end - _head - 1);
    _head = end + 1;
  } else {
    size_t len = 1;

    switch (currChar) {
    case '+':
      if (next() == '+') {
        len = 2;
        result.type = Increment;
      } else {
        result.type = Plus;
      }
      break;
    case '-':
      if (next() == '-') {
        len = 2;
        result.type = Decrement;
      } else if (next() == '>') {
        len = 2;
        result.type = Arrow;
      } else {
        result.type = Minus;
      }
      break;
    // TODO: & | && || += -= *= /= &&= ||=
    case '*': result.type = Star; break;
    case '!': result.type = Not; break;
    case '{': result.type = LBracket; break;
    case '}': result.type = RBracket; break;
    case '(': result.type = LParen; break;
    case ')': result.type = RParen; break;
    case '[': result.type = LSquare; break;
    case ']': result.type = RSquare; break;
    case '.': result.type = Dot; break;
    case ';': result.type = Semicolon; break;
    case '=': result.type = Equal; break;
    }
    result.span = std::string_view(_head, len);
    _head += len;
  }

  if (expected != AnyToken && result.type != expected) {
    cerr << color::boldred("ERROR") << ": Expected token of type " << expected
         << ", but got " << result.type << "!";
    exit(1);
  }
  return result;
}

void Lexer::eatToken(TokenType expected) { nextToken(expected); }

bool Lexer::_isEOF() { return _head >= _src + _length; }

void Lexer::_skipWhitespace() {
  while (!_isEOF() && isspace(*_head)) {
    _head++;
  }
}

bool isOneOf(char c, const std::string symbols) {
  for (char s : symbols) {
    if (c == s) return true;
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
