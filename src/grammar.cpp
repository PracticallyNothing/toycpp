#include "grammar.hpp"

#include "color.hpp"
#include "lex.hpp"
#include "utils.hpp"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iomanip>
#include <ios>
#include <iostream>
#include <iterator>
#include <map>
#include <optional>
#include <ostream>
#include <set>
#include <stack>
#include <string_view>
#include <thread>
#include <type_traits>
#include <vector>

namespace grammar {
// using ast::Node, lex::Lexer;

} // namespace grammar

using std::string, std::vector, std::set, std::map, std::optional;

template<typename T>
std::ostream &operator<<(std::ostream &os, const set<T> &set);
template<typename T>
std::ostream &operator<<(std::ostream &os, const vector<T> &vector);
template<typename K, typename V>
std::ostream &operator<<(std::ostream &os, const map<K, V> &map);

template<typename K, typename V>
std::ostream &operator<<(std::ostream &os, const map<K, V> &map) {
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
  for (int i = 0; i < vector.size(); i++) {
    os << vector[i];
    if (i < vector.size() - 1) os << ", ";
  }
  os << "]";
  return os;
}

template<typename T>
std::ostream &operator<<(std::ostream &os, const set<T> &set) {
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

struct Node {
  string name;
  vector<Node> children;
  bool isTerminal = false;
};

struct UnresolvedRule {
  string ruleName;
};

enum RuleTargetType { RT_TerminalToken, RT_String, RT_Rule };

std::ostream &operator<<(std::ostream &os, TerminalToken token) {
  switch (token) {
  case TT_Invalid       : os << "<?invalid-token?>"; break;
  case TT_Empty         : os << "ε"; break;
  case TT_IntegerLiteral: os << "<IntLiteral>"; break;
  case TT_FloatLiteral  : os << "<FloatLiteral>"; break;
  case TT_DoubleLiteral : os << "<DoubleLiteral>"; break;
  case TT_CharLiteral   : os << "<CharLiteral>"; break;
  case TT_StringLiteral : os << "<StringLiteral>"; break;
  case TT_Identifier    : os << "<Identifier>"; break;
  case TT_Eof           : os << "<EOF>"; break;
  }
  return os;
}

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
        if (token == TT_Identifier) {
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

map<string, Rule> parseGrammarFile(const string filename) {
  using std::ifstream, std::cout, std::cerr, std::endl;

  ifstream file(filename);
  if (!file.is_open() || !file.good()) {
    cerr << "ERROR: Failed to read or open ''" << filename << "'!";
    exit(1);
  }

  auto rulesText = slurp(file);

  lex::Lexer ruleLexer(rulesText);
  lex::Token nextToken;

  bool insideRule = false;

  set<string> unresolvedRules;
  map<string, Rule> rules;
  Rule *currRule = nullptr;

  nextToken = ruleLexer.nextToken();
  while (nextToken.type != lex::Eof) {
    if (!insideRule) {
      string newRuleName(nextToken.span);
      cout << "New rule: " << newRuleName << endl;

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
        cout << "  New alternative for '" << currRule->name << "'." << endl;
        currRule->alternatives.push_back({});
      } else if (nextToken.span == ";") {
        cout << "Done parsing '" << currRule->name << "' - got "
             << currRule->alternatives.size() << " alternatives." << endl;
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
        cerr << "ERROR: Unexpected token " << nextToken
             << "! Expected Identifier, StringLiteral, ; or |." << endl;
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

  return rules;
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

std::ostream &operator<<(std::ostream &os, const Rule::Target &target) {
  switch (target.type) {
  case RT_TerminalToken: os << target.token; break;
  case RT_String       : os << "'" << target.str << "'"; break;
  case RT_Rule         : os << target.str; break;
  }
  return os;
}
std::ostream &operator<<(std::ostream &os, const DottedRule &rule) {
  os << rule.ruleName << " ->";
  for (int i = 0; i < rule.alternative->size(); i++) {
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
  int state;
  map<Rule::Target, int> shifts;
  StupidSet<Reduction> reductions;
};

vector<ParseRules> buildParseTable(const map<string, Rule> &rules,
                                   const map<string, set<Rule::Target>> &firstSets,
                                   const map<string, set<Rule::Target>> &followSets) {
  using std::cout, std::endl;
  cout << ">> Building rules table..." << endl;

  vector<StupidSet<DottedRule>> states;

  // For each state, which non-terminal leads to what next state.
  vector<map<Rule::Target, int>> shifts;
  vector<StupidSet<Reduction>> reductions;

  // Push the initial T/S' rule, which will just resolve to "program".
  Rule::AlternativeT programRule{Rule::Target(rules.at("program"))};
  states.push_back({{.dotPosition = 0, .ruleName = "T", .alternative = &programRule}});

  // Generate all states.
  for (int i = 0; i < states.size(); i++) {
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

    // Create new sets of states.
    map<Rule::Target, int> currShifts;
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

  // Visualize the parsing table.
  for (int i = 0; i < states.size(); i++) {
    std::cout << "[---------------= " << i << " =---------------]" << std::endl;
    for (const auto &dottedRule : states[i]) {
      std::cout << dottedRule << std::endl;
    }

    if (shifts[i].size() > 0) std::cout << "Shifts:" << std::endl;
    for (const auto &kv : shifts[i]) {
      std::cout << "  See " << kv.first << "? SHIFT and goto state " << kv.second
                << std::endl;
    }
    if (reductions[i].size() > 0) std::cout << "Reductions:" << std::endl;
    for (const auto &reduction : reductions[i]) {
      std::cout << "  REDUCE " << reduction.numPop << " -> \033[1m"
                << reduction.ruleName << "\033[0m\n";
    }
  }

  vector<ParseRules> result;
  for (int i = 0; i < states.size(); i++) {
    result.push_back(ParseRules{
        .state = i,
        .shifts = shifts[i],
        .reductions = reductions[i],
    });
  }
  return result;
}

map<string, set<Rule::Target>> buildFirstSets(const map<string, Rule> &rules) {
  // Generate the FIRST sets
  /// The FIRST sets (which terminal can this rule start with?) for each non-terminal.
  map<string, set<Rule::Target>> firstSets;
  map<string, set<string>> firstDependencies;

  // 1. Seed firstSets with all terminals + record all dependencies between rules.
  for (const auto kv : rules) {
    const auto &rule = kv.second;
    firstSets[rule.name] = {};

    for (const auto &alternative : rule.alternatives) {
      if (alternative.empty()) {
        continue;
      }

      for (const auto &target : alternative) {
        if (target.isTerminal()) {
          firstSets[rule.name].insert(target);
          break;
        }

        const auto &dependentRule = rules.at(target.str);

        // Make sure we don't add a dependency to ourselves!
        if (rule.name != target.str) {
          firstDependencies[rule.name].insert(target.str);
        }

        bool dependentAllowsEmpty = std::any_of(
            dependentRule.alternatives.begin(), dependentRule.alternatives.end(),
            [](const Rule::AlternativeT &alternative) { return alternative.empty(); });

        if (!dependentAllowsEmpty) break;
      }
    }
  }

  // 2. Keep resolving dependencies until we've stopped adding new rules.
  bool added = false;
  do {
    added = false;

    for (auto kv : firstDependencies) {
      auto ruleName = kv.first;
      auto dependents = kv.second;

      for (const auto &dependentRuleName : dependents) {
        assert(dependentRuleName != ruleName); // Just to be sure.

        const set<Rule::Target> &oldSet = firstSets[ruleName];
        set<Rule::Target> newSet = oldSet;
        newSet.insert(firstSets[dependentRuleName].begin(),
                      firstSets[dependentRuleName].end());

        if (oldSet.size() < newSet.size()) {
          firstSets[ruleName] = newSet;
          added = true;
        }
      }
    }
  } while (added);

  return firstSets;
}

map<string, set<Rule::Target>>
buildFollowSets(const map<string, Rule> &rules,
                const map<string, set<Rule::Target>> &firstSets) {
  using namespace std;

  // Generate the FOLLOW sets.
  // The FOLLOW sets (which terminal appears after the end of this rule?) for each
  // non-terminal.
  map<string, set<Rule::Target>> followSets;
  map<string, set<string>> followDependencies;

  // 1. Seed followSets with terminals and FIRST-sets. Keep track of dependencies.
  for (const auto &kv : rules) {
    const auto &ruleName = kv.first;

    for (const auto &otherKv : rules) {
      const auto &alternatives = otherKv.second.alternatives;

      for (const auto &alternative : alternatives) {
        for (int i = 0; i < alternative.size(); i++) {
          const auto &target = alternative[i];

          if (target.isTerminal() || target.str != ruleName) {
            continue;
          }

          if (i == alternative.size() - 1) {
            followDependencies[ruleName].insert(otherKv.first);
            continue;
          }

          i++;
          for (; i < alternative.size(); i++) {
            const auto &next = alternative[i];

            if (next.isTerminal()) {
              // If this is a terminal, add it to the follow set and stop.
              followSets[ruleName].insert(next);
              break;
            } else if (next.str == ruleName) {
              // If this target is the same terminal (e.g. expression -> expression "+"
              // expression;) stop processing it - let the outer loop handle it.
              i--;
              break;
            } else {
              const auto &dependent = rules.at(next.str);
              followSets[ruleName].insert(firstSets.at(dependent.name).begin(),
                                          firstSets.at(dependent.name).end());

              bool isLast = i == alternative.size() - 1;
              bool dependentAllowsEmpty = std::any_of(
                  dependent.alternatives.begin(), dependent.alternatives.end(),
                  [](const auto &alternative) { return alternative.empty(); });

              if (!dependentAllowsEmpty) break;
              if (isLast) followDependencies[ruleName].insert(otherKv.first);

              followDependencies[ruleName].insert(next.str);
            }
          }
        }
      }
    }
  }

  // 2. Keep resolving dependencies until we've stopped adding new rules.
  bool added = false;
  do {
    added = false;

    for (auto kv : followDependencies) {
      auto ruleName = kv.first;
      auto dependents = kv.second;

      for (const auto &dependentRuleName : dependents) {
        const set<Rule::Target> &oldSet = followSets[ruleName];
        set<Rule::Target> newSet = oldSet;
        newSet.insert(followSets[dependentRuleName].begin(),
                      followSets[dependentRuleName].end());

        if (oldSet.size() < newSet.size()) {
          followSets[ruleName] = newSet;
          added = true;
        }
      }
    }
  } while (added);

  return followSets;
}

class Parser {
public:
  Parser(vector<ParseRules> rules) : rules(rules) {}

  bool done() const { return isDone; }

  bool advance(lex::Token lookahead) {
    // The latest produced node. Will enter `nodes` once shifted.
    bool consumedLookahead = false;

    bool reduced = false;

    while (!states.empty()) {
      std::cout << "State: " << currState() << ", nodes: [";
      for (const auto &node : nodes) {
        std::cout << " "
                  << (node.isTerminal ? "\033[3m'" + node.name + "'\033[0m"
                                      : node.name);
      }
      if (latestReduction.has_value())
        std::cout << " \033[4m" << latestReduction.value().name << "\033[0m";
      if (!consumedLookahead) std::cout << " $ \033[4m" << lookahead.span << "\033[0m";
      std::cout << " ]\n";

      // Try shifting.
      auto matchingShift =
          std::find_if(currShifts().begin(), currShifts().end(), [&](auto kv) {
            const auto &target = kv.first;
            if (latestReduction.has_value())
              return target.matches(latestReduction.value());
            else
              return !consumedLookahead && target.matches(lookahead);
          });

      if (matchingShift != currShifts().end()) {
        // Decide whether to consume the lookahead or the latest reduction.
        if (matchingShift->first.matches(lookahead)) {
          assert(!consumedLookahead);
          nodes.push_back(Node{.name = string(lookahead.span), .isTerminal = true});
          consumedLookahead = true;
        } else {
          nodes.push_back(latestReduction.value());
          latestReduction.reset();
        }

        std::cout << "SHIFT \033[4m" << nodes.back().name << "\033[0m, goto state "
                  << matchingShift->second << "\n";

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
        std::cout
            << "Shift/Reduce conflict! Resolving by waiting for new lookahead...\n";
        return true;
      } else {
        const auto &reduction = currReductions()[0];

        if (reduction.ruleName == "T") {
          std::cout << "END! Can't parse any more!\n";
          isDone = true;
          return true;
        }

        std::cout << "REDUCE " << reduction.numPop << " -> " << reduction.ruleName
                  << "\n";

        int i = nodes.size() - reduction.numPop;
        Node node{.name = reduction.ruleName, .children = {}};

        if (reduction.numPop > 0 && nodes.at(i).name == reduction.ruleName) {
          node.children = nodes.at(i).children;
          i++;
        }

        for (; i < nodes.size(); i++) {
          node.children.push_back(nodes.at(i));
        }
        nodes.resize(nodes.size() - reduction.numPop);
        latestReduction = node;

        for (int i = 0; i < reduction.numPop; i++)
          states.pop();

        continue;
      }

      if (consumedLookahead && !latestReduction.has_value()) {
        std::cout << ">> " << color::boldgreen("DONE")
                  << " - we've consumed both the lookahead and the latest reduction!\n";
        return true;
      } else {
        std::cerr << color::boldred("ERROR") << ": Unable to reduce or shift!";
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
  inline const map<Rule::Target, int> &currShifts() const { return currRules().shifts; }
  inline const StupidSet<Reduction> &currReductions() const {
    return currRules().reductions;
  }
  optional<Node> latestReduction{};

  bool isDone = false;

  std::stack<int> states{{0}};
  std::vector<Node> nodes;
  vector<ParseRules> rules;
};

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

int main(int argc, const char **argv) {
  using namespace std;

  if (argc != 2) {
    cerr << "ERROR: Not enough/too many args!" << endl;
    exit(2);
  }

  auto grammar = parseGrammarFile(argv[1]);

  auto firstSets = buildFirstSets(grammar);

  // Print the FIRST sets.
  size_t longestRuleName =
      max_element(grammar.begin(), grammar.end(), [](auto a, auto b) {
        return a.first.size() < b.first.size();
      })->first.size();

  cout << "===================================\nFIRST sets:\n" << left;
  for (const auto &kv : firstSets) {
    cout << "  " << setw(longestRuleName + 7) << "FIRST(" + kv.first + ")" << " = "
         << kv.second << "\n";
  }

  auto followSets = buildFollowSets(grammar, firstSets);
  cout << "===================================\nFOLLOW sets:\n" << left;
  for (const auto &kv : followSets) {
    cout << "  " << setw(longestRuleName + 8) << "FOLLOW(" + kv.first + ")" << " = "
         << kv.second << "\n";
  }

  auto table = buildParseTable(grammar, firstSets, followSets);

  using std::ifstream, std::cout, std::cerr, std::endl;

  ifstream file("../test/add.cpp");
  auto source = slurp(file);
  lex::Lexer lexer(source);

  Parser parser(table);
  while (!parser.done()) {
    cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";
    if (!parser.advance(lexer.nextToken())) {
      exit(4);
    }
  }

  printNodeTree(parser.top());

  return 0;
}
