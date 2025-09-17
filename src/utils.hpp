#pragma once

#include <cstddef>

/// Helper for using std::visit.
template <class... Ts> struct Overloaded : Ts... {
  using Ts::operator()...;
};

/// Deduction guide for Overloaded's operator().
template <class... Ts> Overloaded(Ts...) -> Overloaded<Ts...>;
