#include "doctest.h"
#include "run-helpers.h"

TEST_SUITE_BEGIN("lib-math");

// todo: error cases

TEST_CASE("add")
{
    testStringsTrue({
            {"no args", "(= (+) 0)"},
            {"one positive arg", "(= (+ 4) 4)"},
            {"one negative arg", "(= (+ -4) -4)"},
            {"two positive arg", "(= (+ 3 4) 7)"},
            {"positive and negative", "(= (+ 2 -4) -2)"},
            {"negative and positive ", "(= (+ -4 2) -2)"},
            {"three numbers 1", "(= (+ -4 2 3) 1)"},
            {"three numbers 2", "(= (+ 2 3 -4) 1)"},
            {"three numbers 3", "(= (+ 2 3 4) 9)"},
    });
}

TEST_CASE("sub")
{
    testStringLibError({"no args", "(-)"});
    testStringsTrue({
            {"one arg", "(= (- 4) -4)"},
            {"two args", "(= (- 4 3) 1)"},
            {"three args", "(= (- 4 2 1) 1)"},
    });
}

TEST_CASE("mul")
{
    testStringsTrue({
            {"no args", "(= (*) 1)"},
            {"one positive arg", "(= (* 4) 4)"},
            {"one negative arg", "(= (* -4) -4)"},
            {"two positive arg", "(= (* 3 4) 12)"},
            {"positive and negative", "(= (* 2 -4) -8)"},
            {"negative and positive ", "(= (* -4 2) -8)"},
            {"three numbers 1", "(= (* -4 2 3) -24)"},
            {"three numbers 2", "(= (* 2 3 -4) -24)"},
            {"three numbers 3", "(= (* 2 3 4) 24)"},
    });
}

TEST_CASE("inc")
{
    testStringsLibError({
            {"no args", "(inc)"},
            {"two args", "(inc 3 4)"},
            {"non-numeric 1", "(inc :a)"},
            {"non-numeric 2", "(inc \\a)"},
    });
    testStringsTrue({
            {"positive", "(= (inc 4) 5)"},
            {"negative", "(= (inc -4) -3)"},
    });
}

TEST_CASE("op lt")
{
    testStringsTrue({
            {"one arg", "(= (< 2) true)"},
            {"two args true", "(= (< 2 3) true)"},
            {"two args false", "(= (< 3 2) false)"},
            {"three args true", "(= (< 2 3 1) true)"},
            {"three args false", "(= (< 3 2 5) false)"},
            {"two args eval", "(= (< (inc 2) (inc 3)) true)"},
    });
}

TEST_CASE("op lte")
{
    testStringsTrue({
            {"one arg", "(= (<= 2) true)"},
            {"two args eq", "(= (<= 2 2) true)"},
            {"two args lt", "(= (<= 2 3) true)"},
            {"two args gt", "(= (<= 3 2) false)"},
            {"three args eq", "(= (<= 2 2 1) true)"},
            {"three args lt", "(= (<= 2 3 1) true)"},
            {"three args gt", "(= (<= 3 2 5) false)"},
            {"two args eval", "(= (<= (inc 2) (inc 3)) true)"},
    });
}

TEST_CASE("op gt")
{
    testStringsTrue({
            {"one arg", "(= (> 2) true)"},
            {"two args true", "(= (> 3 2) true)"},
            {"two args false", "(= (> 2 3) false)"},
            {"three args true", "(= (> 3 2 5) true)"},
            {"three args false", "(= (> 2 3 1) false)"},
            {"two args eval", "(= (> (inc 3) (inc 2)) true)"},
    });
}

TEST_CASE("op gte")
{
    testStringsTrue({
            {"one arg", "(= (>= 2) true)"},
            {"two args eq", "(= (>= 3 3) true)"},
            {"two args gt", "(= (>= 3 2) true)"},
            {"two args lt", "(= (>= 2 3) false)"},
            {"three args eq", "(= (>= 3 3 5) true)"},
            {"three args gt", "(= (>= 3 2 5) true)"},
            {"three args lt", "(= (>= 2 3 1) false)"},
            {"two args eval", "(= (>= (inc 3) (inc 2)) true)"},
    });
}

TEST_SUITE_END();
