#include "catch.hpp"
#include "run-helpers.h"

// todo: test =, not=, not

TEST_CASE("op and", "[lib]")
{
    // todo: test no eval past falsy
    testStringsTrue({
            {"no args", "(= (and) true)"},
            {"one true arg", "(= (and 2) 2)"},
            {"one fals arg", "(= (and false) false)"},
            {"one nil arg", "(= (and nil) nil)"},
            {"first arg nil", "(= (and nil 4) nil)"},
            {"first arg false", "(= (and false 4) false)"},
            {"first arg truthy, last nil", "(= (and 4 nil) nil)"},
            {"first arg truthy, last false", "(= (and 4 false) false)"},
            {"some false arg", "(= (and true 3 \"hi\" \\b false 7) false)"},
            {"some nil arg", "(= (and true 3 nil \"hi\" \\b false 7) nil)"},
            {"all truthy args", "(= (and true 3 \"hi\" \\b :key 7) 7)"},
    });
}

TEST_CASE("op or", "[lib]")
{
    // todo: test no eval past truthy
    testStringsTrue({
            {"no args", "(= (or) nil)"},
            {"one truty arg", "(= (or 2) 2)"},
            {"one false arg", "(= (or false) false)"},
            {"one nil arg", "(= (or nil) nil)"},
            {"no truthy args, last nil", "(= (or false false nil) nil)"},
            {"no truthy args, last false", "(= (or false nil false) false)"},
            {"nil args, last false", "(= (or nil nil false) false)"},
            {"nil arg, last truthy", "(= (or nil 4) 4)"},
            {"false arg, last truthy", "(= (or false 4) 4)"},
            {"first truthy, second nil", "(= (or 4 nil) 4)"},
            {"first truthy, second false", "(= (or 4 false) 4)"},
            {"mixed falsy, one truthy arg 1", "(= (or false nil 4 nil) 4)"},
            {"mixed falsy, one truthy arg 1", "(= (or nil 4 false) 4)"},
    });
}
