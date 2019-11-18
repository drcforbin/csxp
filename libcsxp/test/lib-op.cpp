#include "doctest.h"
#include "run-helpers.h"

TEST_SUITE_BEGIN("lib-op");

TEST_CASE("op eq, neq, not")
{
    testStringsTrue({{"true identity", "(= true true)"},
            {"false identity", "(= false false)"},
            {"not= true", "(not= false true)"},
            {"not= false", "(not= true false)"},
            {"invert false", "(not false)"},
            {"invert true", "(not (not true))"},
            {"invert eq true", "(= (not false) true)"},
            {"invert eq false", "(= (not true) false)"},
            {"invert neq false", "(not= (not false) false)"},
            {"invert neq true", "(not= (not true) true)"},
            {"invert nil", "(not nil)"},
            {"number eq", "(= 1234 1234)"},
            {"number neq", "(not= 1234 1235)"},
            {"string eq", "(= \"string\" \"string\")"},
            {"string partial neq 1", "(not= \"string\" \"strig\")"},
            {"string partial neq 2", "(not= \"string\" \"strin\")"},
            {"string neq longer", "(not= \"string\" \"stringo\")"},
            {"vec eq", "(= [1 2 5] [1 2 5])"},
            {"vec neq differing", "(not= [1 2 5] [1 2 4])"},
            {"vec neq partial", "(not= [1 2 5] [1 2])"},
            {"vec neq longer", "(not= [1 2 5] [1 2 5 6])"},
            {"empty seq eq empty vec", "(= () [])"},
            // todo: map
            // {"empty seq neq empty map",     "(= () {})"},
            // {"empty vec neq empty map",     "(= [] {})"},
            {"empty seq neq non-empty seq", "(not= () '(1))"}});
}

TEST_CASE("op and")
{
    SUBCASE("short circuit")
    {
        // (let [f ([]...)]
        //   (= (and true (f) (f)) true))
        int num_calls = 0;
        auto form = builder::let(
                builder::binding("f",
                        [&num_calls](csxp::Env* env, csxp::AtomIterator* args) -> csxp::patom {
                            num_calls++;
                            return csxp::True;
                        }),
                parseString("(= (and true (f) (f)) true)"));

        // when f returns true, it should be called twice,
        // as and should check both of them, and the whole
        // and expression should return true
        testExpTrue({"counting exp returning true", form});
        REQUIRE(num_calls == 2);

        // (let [f ([]...)]
        //   (= (and true (f) (f)) false))
        num_calls = 0;
        form = builder::let(
                builder::binding("f",
                        [&num_calls](csxp::Env* env, csxp::AtomIterator* args) -> csxp::patom {
                            num_calls++;
                            return csxp::False;
                        }),
                parseString("(= (and true (f) (f)) false)"));

        // when f returns false, it should only be called once,
        // as and should stop checking when it hits a falsy value,
        // and expression should return false
        testExpTrue({"counting exp returning false", form});
        REQUIRE(num_calls == 1);
    }

    SUBCASE("various args")
    {
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
}

TEST_CASE("op or")
{
    SUBCASE("short circuit")
    {
        // (let [f ([]...)]
        //   (= (or false (f) (f)) true))
        int num_calls = 0;
        auto form = builder::let(
                builder::binding("f",
                        [&num_calls](csxp::Env* env, csxp::AtomIterator* args) -> csxp::patom {
                            num_calls++;
                            return csxp::True;
                        }),
                parseString("(= (or false (f) (f)) true)"));

        // when f returns true, it should be called once,
        // as or should stop once it gets a truthy value,
        // and the whole or expression should return true
        testExpTrue({"counting exp returning true", form});
        REQUIRE(num_calls == 1);

        // (let [f ([]...)]
        //   (= (or false (f) (f)) false))
        num_calls = 0;
        form = builder::let(
                builder::binding("f",
                        [&num_calls](csxp::Env* env, csxp::AtomIterator* args) -> csxp::patom {
                            num_calls++;
                            return csxp::False;
                        }),
                parseString("(= (or false (f) (f)) false)"));

        // when f returns false, it should be called twice,
        // as or should keep checking until it hits a truthy value,
        // and the whole or expression should return false
        testExpTrue({"counting exp returning false", form});
        REQUIRE(num_calls == 2);
    }

    SUBCASE("various args")
    {
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
}

TEST_SUITE_END();
