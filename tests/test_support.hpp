#pragma once
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
// Temporary tiny runner until the team selects GoogleTest or Catch2.
// Checks throw in Debug AND Release; do not replace them with assert().
inline void check(bool condition, const char* expression, int line) {
    if (!condition) throw std::runtime_error(std::string("Line ") + std::to_string(line) + ": " + expression);
}
#define CHECK(...) check(static_cast<bool>((__VA_ARGS__)), #__VA_ARGS__, __LINE__)
inline bool near(float a, float b) { return std::abs(a - b) < 0.001F; }
template<class Exception, class F> bool throws_as(F action) {
    try { action(); } catch (const Exception&) { return true; }
    return false;
}
template<class F> int run_test(F test) {
    try { test(); std::cout << "PASS\n"; return 0; }
    catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
