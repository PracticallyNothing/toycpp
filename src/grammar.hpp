#pragma once

// #include "ast.hpp"
// #include "lex.hpp"

// #include <functional>
// #include <initializer_list>
// #include <optional>
// #include <vector>

// namespace grammar {
// using std::vector, std::optional;

// enum ParserState {
//   Fail,       // The parser cannot parse the given sequence.
//   NeedMore,   // The parser needs more tokens to complete its parsing.
//   Reduce,     // The parser is done.
//   RepeatDone, // A special state.
// };

// enum ParserType {
//   Sequence,
//   Choice,
//   Repeat,
// };

// struct ParserAdvanceResult {
//   ParserState state;

//   size_t numTokensConsumed;

//   /// Does this parser produce an ast::Node?
//   bool produces;

//   /// The function to produce an ast::Node.
//   const std::function<ast::Node(vector<ast::Node>)> &produce;
// };

// struct ParseTarget {
//   union {
//     Parser *parser;
//   };
// };

// class Parser {
// public:
//   Parser(ParserType type, std::initializer_list<ParseTarget> targets)
//       : type(type), targets(targets) {}
//   ParserState advance(lex::Token token);

// private:
//   ParserType type;
//   bool canAdvance(lex::Token token) const;

//   /// The things that this Parser needs to match.
//   vector<> targets;
// };
// } // namespace grammar
