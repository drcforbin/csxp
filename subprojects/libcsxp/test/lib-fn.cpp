#include "doctest.h"
#include "run-helpers.h"

TEST_SUITE_BEGIN("lib-fn");

// todo: test defn

TEST_CASE("fn")
{
    testStringsTrue({
            {"inline call fn", "(= ((fn [a] [a a]) 3) [3 3])"},
            {"let fn", R"-(
            (= (let [f (fn [a] [a a])]
                 (f 4)
                 (f 5))
               [5 5])
            )-"},
            {"multiform let", R"-(
            (= (let [f (if false 3 (fn [a] [a a]))]
                 (f 6)
                 (if true (f 33) (f 44)))
               [33 33])
            )-"},
    });
}

TEST_SUITE_END();
