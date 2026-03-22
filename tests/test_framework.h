#pragma once

#include <iostream>
#include <string>
#include <cmath>
#include <functional>
#include <vector>

// Minimal test framework - no external dependencies
namespace test {

static int g_passed = 0;
static int g_failed = 0;
static int g_total = 0;

inline void check(bool condition, const std::string& name) {
    g_total++;
    if (condition) {
        g_passed++;
        std::cout << "  [PASS] " << name << std::endl;
    } else {
        g_failed++;
        std::cout << "  [FAIL] " << name << std::endl;
    }
}

inline void checkFloat(float actual, float expected, const std::string& name, float eps = 1e-4f) {
    check(std::abs(actual - expected) < eps, name + " (" + std::to_string(actual) + " ~= " + std::to_string(expected) + ")");
}

inline int report(const std::string& suiteName) {
    std::cout << "\n=== " << suiteName << " Results ===" << std::endl;
    std::cout << "Passed: " << g_passed << "/" << g_total << std::endl;
    if (g_failed > 0) {
        std::cout << "FAILED: " << g_failed << " test(s)" << std::endl;
        return 1;
    }
    std::cout << "All tests passed!" << std::endl;
    return 0;
}

} // namespace test
