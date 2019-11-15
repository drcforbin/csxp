#include "doctest.h"
#include "run-helpers.h"

// todo: error cases
//   too many / too few rparens
// todo: commented stuff below
// todo: map and vector persistence
// todo: closures in fns
// todo: more string iteration

// todo: add iterator-based tests

// todo: test falses
/*
        {"false neq true",                 "(= true false)"},
        {"false itself",                 "false"},
        {"nil itself",                 "nil"},
        {"not true",                 "(not true)"},
*/
TEST_SUITE_BEGIN("integration");

TEST_CASE("basic eval")
{
    testStringsTrue({
            {"true itself", "true"},
            {"number", "1234"},
            {"string", "\"string\""},
            {"vec", "[1 2 5]"},
            {"empty seq", "()"},
    });
}

TEST_CASE("basic math")
{
    testStringsTrue({
            {"eq inc", "(= 1234 (inc 1233))"},
            {"eq plus", "(= 1234 (+ 1233 1))"},
            {"eq inc plus 1", "(= (inc 1233) (+ 1233 1))"},
            {"eq inc plus 2", "(= (inc 1234) (+ 1234 1))"},
            {"neq num inc 1", "(not= 1234 (inc 1234))"},
            {"neq num inc 2", "(not= 1234 (+ 1234 1))"},
    });
}

TEST_CASE("closures")
{
    testStringsTrue({
            {"enclose one value", R"-(
            (= (let [y 6
                     f (fn [x] (+ x y))]
                 (f 15))
               21)
            )-"},
            {"enclose two value", R"-(
            (= (let [y 6
                     z 4
                     f (fn [x] (+ x y z))]
                 (f 15))
               25)
            )-"},
            {"enclose two levels", R"-(
            (= (let [a 9 b 1]
                 (let [x 6 y 4
                       f (fn [arg] (+ a b x y arg))]
                 (f 15)))
               35)
            )-"},
            {"fn returning fn", R"-(
            (defn make-f [n]
              (fn [x] (+ x n)))
            (= (let [f (make-f 9)]
                 (f 15))
               24)
            )-"},
            {"fn returning fn returning fn", R"-(
            (defn make-f-0 [n]
              (fn [x]
                (fn [y] (+ x y n))))
            (def make-f-1 (make-f-0 6))
            (= (let [f (make-f-1 5)]
                 (f 4))
               15)
            )-"},
    });
}

TEST_CASE("destructure vec")
{
    testStringsTrue({
            // vec
            {"vec destructure vec binding 1", "(= (let [[x y] [9 10]] x) 9)"},
            {"vec destructure vec binding 2", "(= (let [[x y] [9 10]] y) 10)"},
            {"vec destructure seq", "(= (let [[x y] '(9 10)] y) 10)"},
            {"vec destructure missing binding 1", "(= (let [[x y] [9]] x) 9)"},
            {"vec destructure missing binding 2", "(= (let [[x y] [9]] y) nil)"},
            // nested let
            {"nested let binding 1", R"-(
            (= (let [fst (fn [[x y]] x)]
                 (let [[x y] ['(1 2 3) '(9 10 11 12)]]
                   (fst y)))
               9)
            )-"},
            {"nested let binding 2", R"-(
            (= (let [scnd (fn [[x y]] y)]
                 (let [[x y] ['(1 2 3) '(9 10 11 12)]]
                   (scnd y)))
               10)
            )-"},

            // nested vector & / "rest"
            {"nested vec with rest 1", R"-(
            (= (let [fst (fn [[x y]] x)]
                 (let [[x & y] '(9 10 11 12)]
                   (fst y)))
               10)
            )-"},
            {"nested vec with rest 2", R"-(
            (= (let [scnd (fn [[x y]] y)]
                 (let [[x & y] '(9 10 11 12)]
                   (scnd y)))
               11)
            )-"},
            {"nested vec with rest 3", R"-(
            (= (let [scnd (fn [[x y]] y)]
                 (let [[x & y] [9 10 11 12]]
                   (scnd y)))
               11)
            )-"},

            // multi pair let, with pair
            {"let with multiple bindings 1", R"-(
            (= (let [fst (fn [[x y]] x)
                     [x y] ['(1 2 3) '(9 10 11 12)]]
                 (fst y))
               9)
            )-"},

            // multi pair let, with 'rest'
            {"let with multiple bindings 2", R"-(
            (= (let [scnd (fn [[x y]] y)
                     [x & y] [9 10 11 12]]
                 (scnd y))
               11)
            )-"},

            // vector binding :as
            {"vec binding nested let with as", R"-(
            (= (let [scnd (fn [[x y]] y)]
                 (let [[x :as y] '(9 10 11 12)]
                   (assert (= x 9))
                   (scnd y)))
               10)
            )-"},

            // nested bindings, & and :as
            {"vec binding nested let", R"-(
            (= (let [[[x1 y1][x2 y2]] [[1 2] [3 4]]]
                 [x1 y1 x2 y2])
               [1 2 3 4])
            )-"},
            {"vec binding with rest and as 1", R"-(
            (= (let [[a b & c :as v] [5 6 7 8 9 10]]
                 [a b c v])
               [5 6 [7 8 9 10] [5 6 7 8 9 10]])
            )-"},

            /* todo: this is broken too, need iterate str
        // binding string, nested, & and :as, aliasing str
        {"vec binding with rest and as 2", R"-(
        (= (let [[a b & c :as str] "asdjhhfdas"]
             [a b c str])
           [\a \s [\d \j \h \h \f \d \a \s] "asdjhhfdas"])
        )-"},
            */
    });
}

/*
todo: need to implement some sort of data structure
TEST_CASE("destructure map", "[integration]")
{
    testStringsTrue({
        // map bindings
        {"map destructure with as and or", R"-(
        (assert (= (let [{a :a, b :b, c :c, :as m :or {a 2 b 3 c 9}}  {:a 5 :c 6}]
                      [a b c m])
                   [5 3 6 {:c 6, :a 5}]))
        )-"},

        // nested, map, vec, & and :as
        {"nested map destructure with vec, rest and as", R"-(
        (assert (= (let [m {:j 15 :k 16 :ivec [22 23 24 25]}
                         {j :j, k :k, i :i, [r s & t :as v] :ivec, :or {i 12 j 13}} m]
                     [i j k r s t v])
                   [12 15 16 22 23 (24 25) [22 23 24 25]]))
        )-"},
    });
}

TEST_CASE("destructure kws", "[integration]")
{
    testStringsTrue({
        // TODO: :strs and :syms
        {"let with keys", R"-(
        (assert (= (let [m {:a 1, :b 2}
                         {:keys [a b]} m]
                     (+ a b))
                   3))
        )-"},
        {"let with prefixed keys", R"-(
        (assert (= (let [m {:x/a 1, :y/b 2}
                         {:keys [x/a y/b]} m]
                     (+ a b))
                   3))
        )-"},
        {"let with bound keys", R"-(
        (assert (= (let [m {::x 42}
                          {:keys [::x]} m]
                     x)
                   42))
        )-"},
        // TODO: vector :as
        // TODO: map :as
    });
}
*/

// todo: (take 3 (drop 5 (range 1 11)))

TEST_SUITE_END();
