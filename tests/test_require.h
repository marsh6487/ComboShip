#ifndef COMBOSHIP_TEST_REQUIRE_H
#define COMBOSHIP_TEST_REQUIRE_H

#include <cstdio>
#include <cstdlib>

// Native fixtures must keep their assertions enabled in Release builds too.
#define REQUIRE(expression)                                                                                           \
    do {                                                                                                              \
        if (!(expression)) {                                                                                          \
            std::fprintf(stderr, "%s:%d: requirement failed: %s\n", __FILE__, __LINE__, #expression);                  \
            std::abort();                                                                                             \
        }                                                                                                             \
    } while (0)

#endif
