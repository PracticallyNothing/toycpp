#pragma once

#include "color.hpp"

#include <algorithm>
#include <cstddef>
#include <fstream>
#include <initializer_list>
#include <iostream>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

struct Location {
  std::string filename;
  std::string_view fullSpan;

  unsigned startLine;
  unsigned startColumn;

  unsigned endLine;
  unsigned endColumn;
};

/// Helper for using std::visit.
template<class... Ts>
struct Overloaded : Ts... {
  using Ts::operator()...;
};

/// Deduction guide for Overloaded's operator().
template<class... Ts>
Overloaded(Ts...) -> Overloaded<Ts...>;

// Thank you,
// https://stackoverflow.com/a/116220
inline std::string slurp(std::ifstream &in) {
  if (!in.is_open() || !in.good()) {
    abort();
  }

  std::ostringstream sstr;
  sstr << in.rdbuf();
  return sstr.str();
}

/// A set that only includes elements that aren't equal to any other elements in
/// the set. Comparison is done using operator==.
///
/// Backed by a std::vector<T>. Probably very slow.
template<typename T>
class StupidSet {
public:
  StupidSet() {}
  StupidSet(std::initializer_list<T> init) : data(init) {}

  inline size_t size() const { return data.size(); }
  inline bool empty() const { return data.empty(); }

  inline typename std::vector<T>::iterator begin() { return data.begin(); }
  inline typename std::vector<T>::iterator end() { return data.end(); }

  inline typename std::vector<T>::const_iterator begin() const { return data.cbegin(); }
  inline typename std::vector<T>::const_iterator end() const { return data.cend(); }

  inline bool insert(const T &item) {
    if (!contains(item)) {
      data.push_back(item);
      return true;
    }
    return false;
  }
  inline void erase(const T &item) {
    auto iter = find(item);
    if (iter != end()) {
      data.erase(iter);
    }
  }
  inline void clear() { data.clear(); }

  inline size_t merge(const StupidSet<T> &other) {
    size_t countInserted = 0;

    for (const auto &item : other) {
      if (insert(item)) countInserted++;
    }

    return countInserted;
  }

  inline bool operator==(const StupidSet<T> &other) const {
    if (size() != other.size()) return false;

    for (size_t i = 0; i < size(); i++) {
      bool found = false;

      for (size_t j = 0; j < other.size(); j++) {
        if (data[i] == other.data[j]) {
          found = true;
          break;
        }
      }

      if (!found) return false;
    }

    return true;
  }

  inline auto find(const T &item) { return std::find(data.begin(), data.end(), item); }
  inline auto find(const T &item) const {
    return std::find(data.cbegin(), data.cend(), item);
  }
  inline bool contains(const T &item) const { return find(item) != end(); }

  T &operator[](size_t i) { return data[i]; }
  const T &operator[](size_t i) const { return data[i]; }

private:
  std::vector<T> data;
};

inline std::string format(const std::string &s) { return s; }

/// Following fmtStr, create a new string that incorporates args. fmtStr should
/// leave placeholders for things using "{}".
///
/// Copied from SWAN-Game-Engine:
///   https://github.com/PracticallyNothing/SWAN-Game-Engine/blob/master/SWAN/Core/Format.hpp
template<typename First, typename... Args>
std::string format(const std::string &fmtStr, First f, Args &&...args) {
  std::stringstream ss;

  auto fmt = fmtStr.begin();
  while (fmt != fmtStr.end()) {
    if (*fmt == '{') {
      /// Information about the conversion.
      std::stringstream info;
      while (*fmt != '}') {
        ++fmt;
        info << *fmt;
      }
      ++fmt;

      ss << f;
      ss << fmtStr.substr(std::distance(fmtStr.begin(), fmt));
      return format(ss.str(), args...);
    } else {
      ss << *fmt;
      ++fmt;
    }
  }

  return ss.str();
}

enum ReportLevel { INFO, WARNING, ERROR };

/// Report something while including context from the source code.
template<typename... Args>
void reportWithContext(ReportLevel level, Location location, std::string fmt,
                       Args &&...args) {
  using std::cerr, std::endl, std::string;

  cerr << location.filename << ":" << location.startLine << ":" << location.startColumn
       << ": ";

  switch (level) {
  case INFO   : cerr << color::bold("INFO"); break;
  case WARNING: cerr << color::yellow("WARN"); break;
  case ERROR  : cerr << color::boldred("ERROR"); break;
  }

  cerr << ": " << format(fmt, args...) << endl
       << "  " << location.fullSpan << endl
       << "  ";

  for (size_t i = 0; i < location.fullSpan.size(); i++) {
    if (i < location.startColumn - 1 || location.endColumn - 2 < i) {
      cerr << ' ';
    } else {
      cerr << '^';
    }
  }
}
