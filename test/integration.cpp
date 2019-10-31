#include "catch.hpp"
#include "run-helpers.h"

// todo: error cases
//   too many / too few rparens
// todo: commented stuff below
// todo: map and vector persistence
// todo: closures in fns
// todo: more string iteration

// todo: reformat clojure code
// todo: add iterator-based tests

// todo: test falses
/*
        {"false neq true",                 "(= true false)"},
        {"false itself",                 "false"},
        {"nil itself",                 "nil"},
        {"not true",                 "(not true)"},
*/

TEST_CASE("basic eval", "[integration]")
{
    testStringsTrue({{"true itself", "true"},
            {"true identity", "(= true true)"},
            {"false identity", "(= false false)"},
            {"not= trur", "(not= false true)"},
            {"not= false", "(not= true false)"},
            {"invert false", "(not false)"},
            {"invert true", "(not (not true))"},
            {"invert eq true", "(= (not false) true)"},
            {"invert eq false", "(= (not true) false)"},
            {"invert neq false", "(not= (not false) false)"},
            {"invert neq true", "(not= (not true) true)"},
            {"invert nil", "(not nil)"},
            {"number", "1234"},
            {"number eq", "(= 1234 1234)"},
            {"number neq", "(not= 1234 1235)"},
            {"string", "\"string\""},
            {"string eq", "(= \"string\" \"string\")"},
            {"string partial neq 1", "(not= \"string\" \"strig\")"},
            {"string partial neq 2", "(not= \"string\" \"strin\")"},
            {"string neq longer", "(not= \"string\" \"stringo\")"},
            {"vec", "[1 2 5]"},
            {"vec eq", "(= [1 2 5] [1 2 5])"},
            {"vec neq differing", "(not= [1 2 5] [1 2 4])"},
            {"vec neq partial", "(not= [1 2 5] [1 2])"},
            {"vec neq longer", "(not= [1 2 5] [1 2 5 6])"},
            {"empty seq", "()"},
            {"empty seq eq empty vec", "(= () [])"},
            // todo: map
            // {"empty seq neq empty map",     "(= () {})"},
            // {"empty vec neq empty map",     "(= [] {})"},
            {"empty seq neq non-empty seq", "(not= () '(1))"}});
}

TEST_CASE("basic math", "[integration]")
{
    testStringsTrue({
            {"eq inc", "(= 1234 (inc 1233))"},
            {"eq plus", "(= 1234 (+ 1233 1))"},
            {"eq inc plus 1", "(= (inc 1233) (+ 1233 1))"},
            {"eq inc plus 2", "(= (inc 1234) (+ 1234 1))"},
            {"neq num inc 1", "(not= 1234 (inc 1234))"},
            {"neq num inc 2", "(not= 1234 (+ 1234 1))"},

            {"add one arg", "(= (+ 4) 4)"},
            {"add two arg", "(= (+ 3 4) 7)"},
            {"add three arg", "(= (+ 2 3 4) 9)"},
            {"mul one arg", "(= (* 4) 4)"},
            {"mul two arg", "(= (* 3 4) 12)"},
            {"mul three arg", "(= (* 2 3 4) 24)"},
    });
}

TEST_CASE("special form def", "[integration]")
{
    testStringsTrue({
            {"def lookup seq", R"-(
            (def L '(a b c))
            (= L '(a b c))
            )-"
             },
            {"def loopup num", R"-(
            (= (def M 1) 'M)
            (= M 1)
            )-"
             },
    });
}

TEST_CASE("special form if", "[integration]")
{
    testStringsTrue({
            {"if false no else", "(= (if false 3) nil)"},
            {"if true no else", "(= (if true 3) 3)"},
            {"if true else", "(= (if true 3 4) 3)"},
            {"if plus", "(= (if (+ 1 2) 3 4) 3)"},
            {"if nil", "(= (if nil 3 4) 4)"},
            {"if empty seq", "(= (if () 3 4) 3)"},
            {"if empty str", "(= (if \"\" 3 4) 3)"},
            {"nested if nils 1", "(= (if (if nil nil nil) 3 4) 4)"},
            {"nested if nils 2", "(= (if (if true nil nil) 3 4) 4)"},
            {"nested if nils 3", "(= (if (if true true nil) 3 4) 3)"},
            {"nested if nils 4", "(= (if (if true nil true) 3 4) 4)"},
            {"if nested in vec", "(= [1 (if true 3 9) 5] [1 3 5])"},
    });
}

TEST_CASE("special form do", "[integration]")
{
    testStringsTrue({
            {"do single item", "(= (do 1) 1)"},
            {"do two items", "(= (do 1 2) 2)"},
            {"do four items", "(= (do 1 2 3 true) true)"},
            {"do nested if", R"-(
            (= (do
                 1
                 (if true 4 5)
                 3
                 (if false 9 8))
               8)
                       )-"
             },
    });
}

TEST_CASE("special form let", "[integration]")
{
    testStringsTrue({
            {"let with num", "(= (let [a 37] a) 37)"},

            // aliasing
            {"aliasing let", R"-(
            (= (let [x 7]
                       (let [x (inc x)]
                         (assert (= x 8)))
                       x)
                     7)
                     )-"
             },
    });
}

TEST_CASE("special form quote", "[integration]")
{
    testStringsTrue({
            {"quote vec", "(= '(1 2 3 4 5 6) [1 2 3 4 5 6])"},
            {"quote seq", "(= (quote (1 2 3 4 5 6)) [1 2 3 4 5 6])"},
            {"quote num 1", "(= (quote 4) 4)"},
            {"quote num 2", "(= '4 4)"},
            {"quote sym 1", "(= (quote A) 'A)"},
            {"quote sym 2", "(= 'A 'A)"},
            {"quote map 1", "(= (quote {}) {})"},
            {"quote map 2", "(= '{} {})"},
            {"quote multi arg 1", "(= (quote 1 2 3 4 5 6) 1)"},
            {"quote multi arg 2", "(= (quote {} {} {}) {})"},
            {"quote multi arg 3", "(= '({} {} {}) (quote ({} {} {})))"},
            {"quote plus 1", "(= (quote (+ 1 2)) '(+ 1 2))"},
            {"quote plus 2", "(= '(+ 1 2) '(+ 1 2))"},
    });
}

TEST_CASE("special form fn", "[integration]")
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

TEST_CASE("destructure vec", "[integration]")
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

/* todo: compare to go benchmark
func BenchmarkParseEval(b *testing.B) {
    for i := 0; i < b.N; i++ {
        TestBasicEval(nil)
        TestBasicMath(nil)
        TestSpecialFormDef(nil)
        TestSpecialFormIf(nil)
        TestSpecialFormDo(nil)
        TestSpecialFormLet(nil)
        TestSpecialFormQuote(nil)
        TestSpecialFormFn(nil)
        TestDestructureVec(nil)
    }
}
*/
