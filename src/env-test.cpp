#include "catch.hpp"
#include "env.h"

#include <array>

TEST_CASE("can eval basic items", "[env]")
{
    std::array atoms = {
        atom::Const::make_atom("alice"),
		atom::Str::make_atom("bob"),
		atom::Char::make_atom('c'),
		atom::Num::make_atom(1000),
	};

	for (auto a : atoms) {
		auto env = createEnv();
		auto res = env->eval(a);
        REQUIRE(*res == *a);
	}
}
