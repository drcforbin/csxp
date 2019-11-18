#include "doctest.h"
#include "csxp/env.h"

#include <array>

TEST_SUITE_BEGIN("env");

TEST_CASE("can eval basic items")
{
    std::array atoms = {
            csxp::Const::make_atom("alice"),
            csxp::Str::make_atom("bob"),
            csxp::Char::make_atom('c'),
            csxp::Num::make_atom(1000),
    };

    for (auto a : atoms) {
        auto env = csxp::createEnv();
        auto res = env->eval(a);
        REQUIRE(*res == *a);
    }
}

TEST_SUITE_END();
