#pragma once

#include <algorithm>
#include <cstddef>
#include <fstream>
#include <initializer_list>
#include <iostream>
#include <ostream>
#include <sstream>
#include <vector>

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
