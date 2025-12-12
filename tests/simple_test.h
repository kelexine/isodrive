#ifndef SIMPLE_TEST_H
#define SIMPLE_TEST_H

#include <iostream>
#include <string>
#include <vector>
#include <functional>

#define ASSERT_TRUE(condition) \
    if (!(condition)) { \
        std::cerr << "Assertion failed: " << #condition << ", file " << __FILE__ << ", line " << __LINE__ << std::endl; \
        return false; \
    }

#define ASSERT_EQ(expected, actual) \
    if ((expected) != (actual)) { \
        std::cerr << "Assertion failed: " << #expected << " == " << #actual << "\n" \
                  << "  Expected: " << (expected) << "\n" \
                  << "  Actual:   " << (actual) << "\n" \
                  << "  File: " << __FILE__ << ", line " << __LINE__ << std::endl; \
        return false; \
    }

typedef std::function<bool()> TestFunc;

struct Test {
    std::string name;
    TestFunc func;
};

static std::vector<Test> tests;

void register_test(std::string name, TestFunc func) {
    tests.push_back({name, func});
}

#define TEST(name) \
    bool name(); \
    struct Register##name { Register##name() { register_test(#name, name); } } register_##name; \
    bool name()

int run_tests() {
    int passed = 0;
    int failed = 0;
    for (const auto& test : tests) {
        std::cout << "Running " << test.name << "... ";
        if (test.func()) {
            std::cout << "PASSED" << std::endl;
            passed++;
        } else {
            std::cout << "FAILED" << std::endl;
            failed++;
        }
    }
    std::cout << "\nResults: " << passed << " passed, " << failed << " failed." << std::endl;
    return failed > 0 ? 1 : 0;
}

#endif
