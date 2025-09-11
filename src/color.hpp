#include <string>

namespace color {
using std::string;

inline string bold(const string &s) { return "\033[1m" + s + "\033[0m"; }

inline string boldred(const string &s) { return "\033[31;1m" + s + "\033[0m"; }
inline string boldyellow(const string &s) {
  return "\033[32;1m" + s + "\033[0m";
}
inline string boldgreen(const string &s) {
  return "\033[33;1m" + s + "\033[0m";
}

inline string red(const string &s) { return "\033[31m" + s + "\033[0m"; }
inline string yellow(const string &s) { return "\033[32m" + s + "\033[0m"; }
inline string green(const string &s) { return "\033[33m" + s + "\033[0m"; }
} // namespace color
