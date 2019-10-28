#include "catch.hpp"

#include "atom.h"
#include "atom-fmt.h"
#include "env.h"
#include "lib/lib.h"
#include "logging.h" // todo: remove
#include "parser.h"
#include "tokenizer.h"

#include <string_view>
#include <unordered_map>

// todo: error cases
//   too many / too few rparens
// todo: commented stuff below
// todo: map and vector persistence
// todo: closures in fns
// todo: more string iteration

// todo: reformat clojure code
// todo: add iterator-based tests

struct clostringtest {
    std::string name;
    std::string code;
};

atom::patom runString(std::string_view str) {
    // set up basic env
    auto env = createEnv();
    lib::addCore(env.get());
    lib::addMath(env.get());

    auto tz = createTokenizer(str, "internal-test");
    auto p = createParser(tz);

    atom::patom res = atom::Nil;
    while (p->next()) {
        res = env->eval(p->value());
    }

    return res;
}

void testStringTrue(std::string_view name, clostringtest test) {
    INFO(name);
    auto res = runString(test.code);
    REQUIRE(atom::truthy(res));
}

void testStringsTrue(std::unordered_map<std::string, clostringtest> tests) {
    for (auto [name, test] : tests) {
        testStringTrue(name, test);
    }
}

// todo: test falses
/*
        {"false neq true",                 {code: "(= true false)"}},
        {"false itself",                 {code: "false"}},
        {"nil itself",                 {code: "nil"}},
        {"not true",                 {code: "(not true)"}},
*/

TEST_CASE("basic eval", "[integration]")
{
    testStringsTrue({
        {"true itself",                 {code: "true"}},
        {"true identity",               {code: "(= true true)"}},
        {"false identity",              {code: "(= false false)"}},
        {"not= trur",                   {code: "(not= false true)"}},
        {"not= false",                  {code: "(not= true false)"}},
        {"invert false",                {code: "(not false)"}},
        {"invert true",                 {code: "(not (not true))"}},
        {"invert eq true",              {code: "(= (not false) true)"}},
        {"invert eq false",             {code: "(= (not true) false)"}},
        {"invert neq false",            {code: "(not= (not false) false)"}},
        {"invert neq true",             {code: "(not= (not true) true)"}},
        {"invert nil",                  {code: "(not nil)"}},
        {"number",                      {code: "1234"}},
        {"number eq",                   {code: "(= 1234 1234)"}},
        {"number neq",                  {code: "(not= 1234 1235)"}},
        {"string",                      {code: "\"string\""}},
        {"string eq",                   {code: "(= \"string\" \"string\")"}},
        {"string partial neq 1",        {code: "(not= \"string\" \"strig\")"}},
        {"string partial neq 2",        {code: "(not= \"string\" \"strin\")"}},
        {"string neq longer",           {code: "(not= \"string\" \"stringo\")"}},
        {"vec",                         {code: "[1 2 5]"}},
        {"vec eq",                      {code: "(= [1 2 5] [1 2 5])"}},
        {"vec neq differing",           {code: "(not= [1 2 5] [1 2 4])"}},
        {"vec neq partial",             {code: "(not= [1 2 5] [1 2])"}},
        {"vec neq longer",              {code: "(not= [1 2 5] [1 2 5 6])"}},
        {"empty seq",                   {code: "()"}},
        {"empty seq eq empty vec",      {code: "(= () [])"}},
        // todo: map
        // {"empty seq neq empty map",     {code: "(= () {})"}},
        // {"empty vec neq empty map",     {code: "(= [] {})"}},
        {"empty seq neq non-empty seq", {code: "(not= () '(1))"}}
    });
}

TEST_CASE("basic math", "[integration]")
{
    testStringsTrue({
        {"eq inc",        {code: "(= 1234 (inc 1233))"}},
        {"eq plus",       {code: "(= 1234 (+ 1233 1))"}},
        {"eq inc plus 1", {code: "(= (inc 1233) (+ 1233 1))"}},
        {"eq inc plus 2", {code: "(= (inc 1234) (+ 1234 1))"}},
        {"neq num inc 1", {code: "(not= 1234 (inc 1234))"}},
        {"neq num inc 2", {code: "(not= 1234 (+ 1234 1))"}},

        {"add one arg", {code: "(= (+ 4) 4)"}},
        {"add two arg", {code: "(= (+ 3 4) 7)"}},
        {"add three arg", {code: "(= (+ 2 3 4) 9)"}},
        {"mul one arg", {code: "(= (* 4) 4)"}},
        {"mul two arg", {code: "(= (* 3 4) 12)"}},
        {"mul three arg", {code: "(= (* 2 3 4) 24)"}},
    });
}

TEST_CASE("special form def", "[integration]")
{
    testStringsTrue({
        {"def lookup seq", {
            code: R"-(
            (def L '(a b c))
            (= L '(a b c))
            )-"}},
        {"def loopup num", {
            code: R"-(
            (= (def M 1) 'M)
            (= M 1)
            )-"}},
    });
}

TEST_CASE("special form if", "[integration]")
{
    testStringsTrue({
        {"if false no else", {code: "(= (if false 3) nil)"}},
        {"if true no else",  {code: "(= (if true 3) 3)"}},
        {"if true else",     {code: "(= (if true 3 4) 3)"}},
        {"if plus",          {code: "(= (if (+ 1 2) 3 4) 3)"}},
        {"if nil",           {code: "(= (if nil 3 4) 4)"}},
        {"if empty seq",     {code: "(= (if () 3 4) 3)"}},
        {"if empty str",     {code: "(= (if \"\" 3 4) 3)"}},
        {"nested if nils 1", {code: "(= (if (if nil nil nil) 3 4) 4)"}},
        {"nested if nils 2", {code: "(= (if (if true nil nil) 3 4) 4)"}},
        {"nested if nils 3", {code: "(= (if (if true true nil) 3 4) 3)"}},
        {"nested if nils 4", {code: "(= (if (if true nil true) 3 4) 4)"}},
        {"if nested in vec", {code: "(= [1 (if true 3 9) 5] [1 3 5])"}},
    });
}

TEST_CASE("special form do", "[integration]")
{
    testStringsTrue({
        {"do single item", {code: "(= (do 1) 1)"}},
        {"do two items",   {code: "(= (do 1 2) 2)"}},
        {"do four items",  {code: "(= (do 1 2 3 true) true)"}},
        {"do nested if", {
            code: R"-(
            (= (do
                 1
                 (if true 4 5)
                 3
                 (if false 9 8))
               8)
                       )-"}},
    });
}

TEST_CASE("special form let", "[integration]")
{
    testStringsTrue({
        {"let with num", {code: "(= (let [a 37] a) 37)"}},

        // aliasing
        {"aliasing let", {
            code: R"-(
            (= (let [x 7]
                       (let [x (inc x)]
                         (assert (= x 8)))
                       x)
                     7)
                     )-"}},
    });
}

TEST_CASE("special form quote", "[integration]")
{
    testStringsTrue({
        {"quote vec", {code: "(= '(1 2 3 4 5 6) [1 2 3 4 5 6])"}},
        {"quote seq", {code: "(= (quote (1 2 3 4 5 6)) [1 2 3 4 5 6])"}},
        {"quote num 1", {code: "(= (quote 4) 4)"}},
        {"quote num 2", {code: "(= '4 4)"}},
        {"quote sym 1", {code: "(= (quote A) 'A)"}},
        {"quote sym 2", {code: "(= 'A 'A)"}},
        {"quote map 1", {code: "(= (quote {}) {})"}},
        {"quote map 2", {code: "(= '{} {})"}},
        {"quote multi arg 1", {code: "(= (quote 1 2 3 4 5 6) 1)"}},
        {"quote multi arg 2", {code: "(= (quote {} {} {}) {})"}},
        {"quote multi arg 3", {code: "(= '({} {} {}) (quote ({} {} {})))"}},
        {"quote plus 1", {code: "(= (quote (+ 1 2)) '(+ 1 2))"}},
        {"quote plus 2", {code: "(= '(+ 1 2) '(+ 1 2))"}},
    });
}

TEST_CASE("special form fn", "[integration]")
{
    testStringsTrue({
        {"inline call fn", {code: "(= ((fn [a] [a a]) 3) [3 3])"}},
        {"let fn", {code: R"-(
        (= (let [f (fn [a] [a a])]
            (f 4)
            (f 5))
          [5 5])
          )-"}},
        {"multiform let", {code: R"-(
        (= (let [f (if false 3 (fn [a] [a a]))]
            (f 6)
            (if true (f 33) (f 44)))
          [33 33])
          )-"}},
    });
}

TEST_CASE("destructure vec", "[integration]")
{
    testStringsTrue({
        // vec
        {"vec destructure vec binding 1",     {code: "(= (let [[x y] [9 10]] x) 9)"}},
        {"vec destructure vec binding 2",     {code: "(= (let [[x y] [9 10]] y) 10)"}},
        {"vec destructure seq",               {code: "(= (let [[x y] '(9 10)] y) 10)"}},
        {"vec destructure missing binding 1", {code: "(= (let [[x y] [9]] x) 9)"}},
        {"vec destructure missing binding 2", {code: "(= (let [[x y] [9]] y) nil)"}},

        // nested let
        {"nested let binding 1", {code: R"-(
            (= (let [fst (fn [[x y]] x)]
             (let [[x y] ['(1 2 3) '(9 10 11 12)]]
               (fst y)))
           9)
           )-"}},
        {"nested let binding 2", {code: R"-(
        (= (let [scnd (fn [[x y]] y)]
             (let [[x y] ['(1 2 3) '(9 10 11 12)]]
               (scnd y)))
           10)
           )-"}},

        // nested vector & / "rest"
        {"nested vec with rest 1", {code: R"-(
        (= (let [fst (fn [[x y]] x)]
             (let [[x & y] '(9 10 11 12)]
               (fst y)))
           10)
           )-"}},
        {"nested vec with rest 2", {code: R"-(
        (= (let [scnd (fn [[x y]] y)]
             (let [[x & y] '(9 10 11 12)]
               (scnd y)))
           11)
           )-"}},
        {"nested vec with rest 3", {code: R"-(
        (= (let [scnd (fn [[x y]] y)]
             (let [[x & y] [9 10 11 12]]
               (scnd y)))
           11)
           )-"}},

        // multi pair let, with pair
        {"let with multiple bindings 1", {code: R"-(
        (= (let [fst (fn [[x y]] x)
                 [x y] ['(1 2 3) '(9 10 11 12)]]
             (fst y))
           9)
           )-"}},

        // multi pair let, with 'rest'
        {"let with multiple bindings 2", {code: R"-(
        (= (let [scnd (fn [[x y]] y)
                 [x & y] [9 10 11 12]]
             (scnd y))
           11)
           )-"}},

        // vector binding :as
        {"vec binding nested let with as", {code: R"-(
        (= (let [scnd (fn [[x y]] y)]
             (let [[x :as y] '(9 10 11 12)]
               (assert (= x 9))
               (scnd y)))
           10)
           )-"}},

        // nested bindings, & and :as
        {"vec binding nested let", {code: R"-(
        (= (let [[[x1 y1][x2 y2]] [[1 2] [3 4]]]
             [x1 y1 x2 y2])
           [1 2 3 4])
           )-"}},
        {"vec binding with rest and as 1", {code: R"-(
        (= (let [[a b & c :as v] [5 6 7 8 9 10]]
             [a b c v])
           [5 6 [7 8 9 10] [5 6 7 8 9 10]])
           )-"}},

    /* todo: this is broken too, need iterate str
        // binding string, nested, & and :as, aliasing str
        {"vec binding with rest and as 2", {code: R"-(
        (= (let [[a b & c :as str] "asdjhhfdas"]
             [a b c str])
            [\a \s [\d \j \h \h \f \d \a \s] "asdjhhfdas"])
            )-"}},
            */
    });

    BENCHMARK("complicated binding") {
        return runString(R"-(
          ;(and
            ; let with multiple bindings
            (= (let [fst (fn [[x y]] x)
                     [x y] ['(1 2 3) '(9 10 11 12)]]
                 (fst y))
               9)
            ; let with multiple bindings 2
            (= (let [scnd (fn [[x y]] y)
                     [x & y] [9 10 11 12]]
                 (scnd y))
               11)
            ; vec binding nested let with as
            (= (let [scnd (fn [[x y]] y)]
                 (let [[x :as y] '(9 10 11 12)]
                   (assert (= x 9))
                   (scnd y)))
               10)
            ; vec binding nested let
            (= (let [[[x1 y1][x2 y2]] [[1 2] [3 4]]]
                 [x1 y1 x2 y2])
               [1 2 3 4])
            ;)
           )-");
    };
}

/*
todo: need to implement some sort of data structure
TEST_CASE("destructure map", "[integration]")
{
    testStringsTrue({
        // map bindings
        {"map destructure with as and or", {code: R"-(
        (assert (= (let [{a :a, b :b, c :c, :as m :or {a 2 b 3 c 9}}  {:a 5 :c 6}]
                      [a b c m])
                    [5 3 6 {:c 6, :a 5}]))
                    )-"}},

        // nested, map, vec, & and :as
        {"nested map destructure with vec, rest and as", {code: R"-(
        (assert (= (let [m {:j 15 :k 16 :ivec [22 23 24 25]}
                          {j :j, k :k, i :i, [r s & t :as v] :ivec, :or {i 12 j 13}} m]
                      [i j k r s t v])
                    [12 15 16 22 23 (24 25) [22 23 24 25]]))
                    )-"}},
    });
}

TEST_CASE("destructure kws", "[integration]")
{
    testStringsTrue({
        // TODO: :strs and :syms
        {"let with keys", {code: R"-(
        (assert (= (let [m {:a 1, :b 2}
                          {:keys [a b]} m]
                      (+ a b))
                    3))
                    )-"}},
        {"let with prefixed keys", {code: R"-(
        (assert (= (let [m {:x/a 1, :y/b 2}
                          {:keys [x/a y/b]} m]
                      (+ a b))
                    3))
                    )-"}},
        {"let with bound keys", {code: R"-(
        (assert (= (let [m {::x 42}
                          {:keys [::x]} m]
                      x)
                    42))
                    )-"}},
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

