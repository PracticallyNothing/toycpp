#include "grammar.hpp"

#include "color.hpp"
#include "lex.hpp"
#include "utils.hpp"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <map>
#include <optional>
#include <ostream>
#include <set>
#include <stack>
#include <string_view>
#include <type_traits>
#include <vector>

using std::string, std::vector, std::set, std::map, std::optional;

namespace grammar {

struct DottedRule;

template<typename T>
std::ostream &operator<<(std::ostream &os, const set<T> &set);
template<typename T>
std::ostream &operator<<(std::ostream &os, const vector<T> &vector);
template<typename K, typename V>
std::ostream &operator<<(std::ostream &os, const map<K, V> &map);

enum TerminalToken {
  TT_Invalid,
  TT_Empty,
  TT_Identifier,
  TT_IntegerLiteral,
  TT_FloatLiteral,
  TT_DoubleLiteral,
  TT_CharLiteral,
  TT_StringLiteral,
  TT_Eof,
};

enum RuleTargetType { RT_TerminalToken, RT_String, RT_Rule };

struct Rule {
  struct Target {
  private:
    Target() {}

  public:
    Target(TerminalToken token) : type(RT_TerminalToken), token(token) {}
    Target(string string) : type(RT_String), str(string) {}
    Target(const Rule &rule) : type(RT_Rule), str(rule.name) {}

    inline bool isTerminal() const { return !isNonTerminal(); }
    inline bool isNonTerminal() const { return type == RT_Rule; }

    bool matches(const Node &node) const { return type == RT_Rule && str == node.name; }
    bool matches(const lex::Token &token) const {
      switch (type) {
        // Tokens can't match entire rules.
      case RT_Rule: return false;

      case RT_TerminalToken:
        switch (this->token) {
        case TT_IntegerLiteral:
        case TT_FloatLiteral:
        case TT_DoubleLiteral : return token.type == lex::NumberLiteral;

        case TT_Identifier   : return token.type == lex::Identifier;
        case TT_CharLiteral  : return token.type == lex::CharLiteral;
        case TT_StringLiteral: return token.type == lex::StringLiteral;
        case TT_Eof          : return token.type == lex::Eof;

        default: return false;
        }

      case RT_String: return token.span == str;
      }
      return false;
    }

    static Target RuleByName(string ruleName) {
      Target t;
      t.type = RT_Rule;
      t.str = ruleName;
      return t;
    }

    inline bool operator==(const Target &other) const {
      if (type != other.type) return false;

      switch (type) {
      case RT_TerminalToken:
        if (token == TT_Identifier && other.token == TT_Identifier) {
          return str == other.str;
        } else {
          return token == other.token;
        }

      case RT_Rule:
      case RT_String: return str == other.str;
      }
      return false;
    }

    RuleTargetType type;
    string str;
    TerminalToken token = TT_Invalid;
  };

  using AlternativeT = vector<Target>;

  string name;
  vector<vector<Target>> alternatives;
};

std::ostream &operator<<(std::ostream &os, const Rule::Target &target);
std::ostream &operator<<(std::ostream &os, const DottedRule &rule);
std::ostream &operator<<(std::ostream &os, TerminalToken token);

struct Grammar {
  map<string, Rule> rules;
};

bool operator<(const Rule::Target &lhs, const Rule::Target &rhs) {
  if (lhs.type != rhs.type) {
    return lhs.type < rhs.type;
  }

  switch (lhs.type) {
  case RT_TerminalToken: return lhs.token < rhs.token;
  case RT_String       :
  case RT_Rule         : return lhs.str < rhs.str;
  }

  assert(false);
}

struct DottedRule {
  unsigned int dotPosition;
  string ruleName;
  const Rule::AlternativeT *alternative;

  bool operator==(const DottedRule &other) const {
    return ruleName == other.ruleName && dotPosition == other.dotPosition &&
           *alternative == *other.alternative;
  }

  bool operator<(const DottedRule &other) const {
    return dotPosition < other.dotPosition || ruleName < other.ruleName;
  }

  optional<Rule::Target> beforeDot() const {
    if (dotPosition < 1)
      return {};
    else
      return {(*alternative)[dotPosition - 1]};
  }
  optional<Rule::Target> afterDot() const {
    if (dotPosition >= alternative->size())
      return {};
    else
      return {(*alternative)[dotPosition]};
  }
};

struct Reduction {
  /// The number of items to pop off the stack when reducing.
  size_t numPop;
  /// The name of the rule to reduce by.
  string ruleName;

  inline bool operator==(const Reduction &other) const {
    return ruleName == other.ruleName && numPop == other.numPop;
  }
};

struct ParseRules {
  size_t state;
  map<Rule::Target, size_t> shifts;
  StupidSet<Reduction> reductions;
};

vector<ParseRules> buildParseTable(const Grammar &grammar) {
  using std::cout, std::endl;
  const auto &rules = grammar.rules;

  vector<StupidSet<DottedRule>> states;

  // For each state, which non-terminal leads to what next state.
  vector<map<Rule::Target, size_t>> shifts;
  vector<StupidSet<Reduction>> reductions;

  // Push the initial T/S' rule, which will just resolve to "program".
  Rule::AlternativeT programRule{Rule::Target(rules.at("program"))};
  states.push_back({{.dotPosition = 0, .ruleName = "T", .alternative = &programRule}});

  // Generate all states.
  for (size_t i = 0; i < states.size(); i++) {
    reductions.push_back({});

    StupidSet<DottedRule> seenRules;
    StupidSet<DottedRule> &currSet = states[i];
    StupidSet<Reduction> &currReductions = reductions[i];
    StupidSet<Rule::Target> terminals;

    // In the current set, try to expand all non-terminals to the right of the dot.
    for (size_t j = 0; j < currSet.size(); j++) {
      const auto &rule = currSet[j];

      auto maybeTarget = rule.afterDot();
      if (!maybeTarget.has_value()) {
        // If the dot is at the end of the rule, this is a REDUCE step.
        currReductions.insert(Reduction{
            .numPop = rule.alternative->size(),
            .ruleName = rule.ruleName,
        });
      } else {
        // This is a SHIFT step.
        auto target = maybeTarget.value();
        terminals.insert(target);

        if (target.isNonTerminal()) {
          auto nonTerminalName = string(target.str);

          for (const auto &alternative : rules.at(nonTerminalName).alternatives) {
            auto newRule = DottedRule{
                .dotPosition = 0,
                .ruleName = nonTerminalName,
                .alternative = &alternative,
            };

            if (!seenRules.contains(newRule)) {
              currSet.insert(newRule);
              seenRules.insert(newRule);
            }
          }
        }
      }
    }

    // Check whether the currSet already exists in another state.
    bool alreadyExists = false;
    size_t matchIndex = 0;
    for (; matchIndex < i; matchIndex++) {
      if (states.at(matchIndex) == currSet) {
        alreadyExists = true;
        break;
      }
    }

    if (alreadyExists) {
      shifts.push_back({});

      for (size_t k = 0; k < i; k++) {
        for (auto kv : shifts[k]) {
          if (kv.second == i) {
            shifts[k][kv.first] = matchIndex;
          }
        }
      }
      continue;
    }

    // Create new sets of states.
    map<Rule::Target, size_t> currShifts;
    for (const auto &target : terminals) {
      StupidSet<DottedRule> rulesForTarget;

      for (const auto &dottedRule : states[i]) {
        if (dottedRule.afterDot() == target) {
          DottedRule newRule = dottedRule;
          newRule.dotPosition++;
          rulesForTarget.insert(newRule);
        }
      }

      if (rulesForTarget.size() > 0) {
        // Check whether a state like rulesForTarget already exists.
        auto it = states.begin();
        for (; it != states.end(); it++) {
          if (rulesForTarget == *it) break;
        }

        if (it != states.end()) {
          currShifts[target] = std::distance(states.begin(), it);
        } else {
          currShifts[target] = states.size();
          states.push_back(rulesForTarget);
        }
      }
    }
    shifts.push_back(currShifts);
  }

  vector<ParseRules> result;
  for (size_t i = 0; i < states.size(); i++) {
    result.push_back(ParseRules{
        .state = i,
        .shifts = shifts[i],
        .reductions = reductions[i],
    });
  }
  return result;
}

class Parser {
public:
  Parser(vector<ParseRules> rules) : rules(rules) {}

  bool done() const { return isDone; }

  bool advance(lex::Token lookahead) {
    bool consumedLookahead = false;

    while (!states.empty()) {
      // Try shifting.
      auto [_, matchingShift] =
          findBest(currShifts().begin(), currShifts().end(), [&](auto kv) {
            const auto &target = kv.first;

            if (latestReduction.has_value() &&
                target.matches(latestReduction.value())) {
              return 30;
            } else if (!consumedLookahead && target.matches(lookahead)) {
              return target.type == RT_String ? 20 : 10;
            } else {
              return -1;
            }
          });

      if (matchingShift != currShifts().end()) {
        // Decide whether to consume the lookahead or the latest reduction.
        if (matchingShift->first.matches(lookahead)) {
          assert(!consumedLookahead);
          nodes.push_back(Node{
              .name = string(lookahead.span),
              .children = vector<Node>(),
              .isTerminal = true,
          });
          consumedLookahead = true;
        } else {
          nodes.push_back(latestReduction.value());
          latestReduction.reset();
        }

        states.push(matchingShift->second);
        continue;
      }

      // Try reducing.
      if (currReductions().size() == 0) {
      } else if (currReductions().size() > 1) {
        std::cerr << "ERROR: Reduce/Reduce conflict in state " << currState() << "!"
                  << std::endl;
        exit(3);
      } else if (consumedLookahead &&
                 std::any_of(currShifts().begin(), currShifts().end(),
                             [](const auto &kv) { return kv.first.isTerminal(); })) {
        return true;
      } else {
        const auto &reduction = currReductions()[0];

        if (reduction.ruleName == "T") {
          isDone = true;
          return true;
        }

        size_t i = nodes.size() - reduction.numPop;
        Node node{.name = reduction.ruleName, .children = {}};

        if (reduction.numPop > 0 && nodes.at(i).name == reduction.ruleName) {
          node.children = nodes.at(i).children;
          i++;
        }

        for (; i < nodes.size(); i++) {
          const auto &n = nodes.at(i);
          if (!n.isTerminal && n.name[0] == '_') {
            for (const auto &nchild : n.children)
              node.children.push_back(nchild);
          } else {
            node.children.push_back(n);
          }
        }
        nodes.resize(nodes.size() - reduction.numPop);
        latestReduction = node;

        for (size_t i = 0; i < reduction.numPop; i++)
          states.pop();

        continue;
      }

      if (consumedLookahead && !latestReduction.has_value()) {
        return true;
      } else {
        vector<Rule::Target> targets;

        for (auto kv : currShifts()) {
          targets.push_back(kv.first);
        }

        reportWithContext(ERROR, lookahead.location, "Unexpected {} - expected {}!",
                          lookahead, targets);
        return false;
      }
    }

    std::cerr << "ERROR: We ran out of states to pop!\n";
    return false;
  }

  const Node &top() { return nodes.back(); }

private:
  int currState() const { return states.top(); }
  inline const ParseRules &currRules() const { return rules[states.top()]; }
  inline const map<Rule::Target, size_t> &currShifts() const {
    return currRules().shifts;
  }
  inline const StupidSet<Reduction> &currReductions() const {
    return currRules().reductions;
  }
  optional<Node> latestReduction{};

  bool isDone = false;

  std::stack<int> states{{0}};
  std::vector<Node> nodes;
  vector<ParseRules> rules;
};

Grammar *parseGrammarFile(const string filename) {
  using std::ifstream, std::cout, std::cerr, std::endl;

  ifstream file(filename);
  if (!file.is_open() || !file.good()) {
    cerr << "ERROR: Failed to read or open ''" << filename << "'!";
    exit(1);
  }

  auto rulesText = slurp(file);

  lex::Lexer ruleLexer(filename, rulesText);
  lex::Token nextToken;

  bool insideRule = false;

  set<string> unresolvedRules;
  map<string, Rule> rules;
  Rule *currRule = nullptr;

  nextToken = ruleLexer.nextToken();
  while (nextToken.type != lex::Eof) {
    if (!insideRule) {
      string newRuleName(nextToken.span);

      Rule newRule{.name = newRuleName, .alternatives = {}};
      newRule.alternatives.push_back({});

      rules[newRuleName] = newRule;
      currRule = &rules[newRuleName];

      unresolvedRules.erase(newRuleName);

      ruleLexer.eatToken(lex::Arrow);
      insideRule = true;
    } else {
      assert(currRule != nullptr);
      auto &alternative = currRule->alternatives.back();

      if (nextToken.type == lex::StringLiteral) {
        Rule::Target newTarget(string(nextToken.span));
        alternative.push_back(newTarget);
      } else if (nextToken.span == "|") {
        currRule->alternatives.push_back({});
      } else if (nextToken.span == ";") {
        insideRule = false;
      } else if (nextToken.type == lex::Identifier) {
        auto span = nextToken.span;

        if (span == "Identifier") {
          alternative.push_back(TT_Identifier);
        } else if (span == "IntegerLiteral") {
          alternative.push_back(TT_IntegerLiteral);
        } else if (span == "FloatLiteral") {
          alternative.push_back(TT_FloatLiteral);
        } else if (span == "DoubleLiteral") {
          alternative.push_back(TT_DoubleLiteral);
        } else if (span == "CharLiteral") {
          alternative.push_back(TT_CharLiteral);
        } else if (span == "StringLiteral") {
          alternative.push_back(TT_StringLiteral);
        } else if (span == "Empty") {
          // NOTE(Mario, 2025-09-25):
          //    Empty isn't a token - it just means we can reduce from 0
          //    preceding tokens.
        } else if (span == "Eof") {
          alternative.push_back(TT_Eof);
        } else {
          string ruleName = string(span);

          if (rules.count(ruleName) == 0) {
            unresolvedRules.insert(ruleName);
          }
          alternative.push_back(Rule::Target::RuleByName(ruleName));
        }
      } else {
        reportWithContext(ERROR, nextToken.location,
                          "Expected Identifier, StringLiteral, ; or |, but got {}!",
                          nextToken);
        exit(1);
      }
    }

    nextToken = ruleLexer.nextToken();
  }

  if (unresolvedRules.size() > 0) {
    cerr << "ERROR: The following rules are still unresolved!" << endl;
    for (const auto &ruleName : unresolvedRules) {
      cerr << "- " << ruleName << endl;
    }
    exit(2);
  }

  return new Grammar{.rules = rules};
}

void printNodeTree(const Node &root) {
  using std::function, std::cout, std::endl;

  // Helper function for recursive printing
  function<void(const Node &, const string &, bool)> printNode =
      [&](const Node &node, const string &prefix, bool isLast) {
        // Print the current node
        cout << prefix;
        cout << (isLast ? "└─ " : "├─ ");
        cout << (node.isTerminal ? "'" + node.name + "'" : node.name) << endl;

        // Print children
        for (size_t i = 0; i < node.children.size(); ++i) {
          bool childIsLast = (i == node.children.size() - 1);
          string childPrefix = prefix + (isLast ? "   " : "│  ");
          printNode(node.children[i], childPrefix, childIsLast);
        }
      };

  // Print the root node (without any prefix or connector)
  std::cout << root.name << std::endl;

  // Print children of root
  for (size_t i = 0; i < root.children.size(); ++i) {
    bool isLast = (i == root.children.size() - 1);
    printNode(root.children[i], "", isLast);
  }
}

Node parse(const Grammar *grammar, lex::Lexer &lexer) {
  using namespace std;

  auto table = buildParseTable(*grammar);

  using std::ifstream, std::cout, std::cerr, std::endl;

  Parser parser(table);
  while (!parser.done()) {
    lex::Token nextToken = lexer.nextToken();
    bool ok = parser.advance(nextToken);

    if (!ok) exit(4);
  }

  return parser.top();
}

template<typename K, typename V>
std::ostream &operator<<(std::ostream &os, const map<K, V> &map) {
  if (map.empty()) {
    os << "{}";
    return os;
  }

  os << "{";

  auto endIt = map.end();
  // endIt--;

  for (auto it = map.begin(); it != map.end(); it++) {
    os << it->first << ": " << it->second;
    if (it != endIt) os << ", ";
  }
  os << "}";
  return os;
}

template<typename T>
std::ostream &operator<<(std::ostream &os, const vector<T> &vector) {
  os << "[";
  for (size_t i = 0; i < vector.size(); i++) {
    os << vector[i];
    if (i < vector.size() - 1) os << ", ";
  }
  os << "]";
  return os;
}

template<typename T>
std::ostream &operator<<(std::ostream &os, const set<T> &set) {
  if (set.empty()) {
    os << "{}";
    return os;
  }

  os << "{";
  auto endIt = set.end();
  endIt--;

  for (auto it = set.begin(); it != set.end(); it++) {
    os << *it;
    if (it != endIt) os << ", ";
  }
  os << "}";
  return os;
}

std::ostream &operator<<(std::ostream &os, const grammar::Rule::Target &target) {
  switch (target.type) {
  case grammar::RT_TerminalToken: os << target.token; break;
  case grammar::RT_String       : os << "'" << target.str << "'"; break;
  case grammar::RT_Rule         : os << target.str; break;
  }
  return os;
}

std::ostream &operator<<(std::ostream &os, const grammar::DottedRule &rule) {
  os << rule.ruleName << " ->";
  for (size_t i = 0; i < rule.alternative->size(); i++) {
    if (rule.dotPosition == i) {
      os << " 💠";
    }
    const auto &target = (*rule.alternative)[i];
    os << " " << target;
  }

  if (rule.dotPosition >= rule.alternative->size()) {
    os << " 💠";
  }
  return os;
}

std::ostream &operator<<(std::ostream &os, grammar::TerminalToken token) {
  switch (token) {
  case grammar::TT_Invalid       : os << "<?invalid-token?>"; break;
  case grammar::TT_Empty         : os << "ε"; break;
  case grammar::TT_IntegerLiteral: os << "<IntLiteral>"; break;
  case grammar::TT_FloatLiteral  : os << "<FloatLiteral>"; break;
  case grammar::TT_DoubleLiteral : os << "<DoubleLiteral>"; break;
  case grammar::TT_CharLiteral   : os << "<CharLiteral>"; break;
  case grammar::TT_StringLiteral : os << "<StringLiteral>"; break;
  case grammar::TT_Identifier    : os << "<Identifier>"; break;
  case grammar::TT_Eof           : os << "<EOF>"; break;
  }
  return os;
}

} // namespace grammar
