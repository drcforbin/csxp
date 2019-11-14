#include "doctest.h"
#include "run-helpers.h"

TEST_SUITE_BEGIN("lib-core");

// todo: error cases
// todo: test ns
// todo: test require

TEST_CASE("if")
{
    testStringsLibError({
            {"empty if", "(if)"},
            {"if true no then", "(if true)"},
    });
    testStringsTrue({
            {"if true", "(= (if true 4 5) 4)"},
            {"if true, no false", "(= (if true 4) 4)"},
            {"if false", "(= (if false 4 5) 5)"},
            {"if false, no false", "(= (if false 4) nil)"},
            {"evaled", "(= (if (= 3 (inc 2)) (inc 3) (inc 4)) 4)"},
    });
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

TEST_CASE("when")
{
    // todo: keyword arg, wrong # args
    testStringsTrue({
            {"when true", "(= (when true 4) 4)"},
            {"when true empty", "(= (when true) nil)"},
            {"when false empty", "(= (when false) nil)"},
            {"when true multiple", "(= (when true (inc 9) 4) 4)"},
            {"when false", "(= (when false 4) nil)"},
            {"evaled", "(= (when (= 3 (inc 2)) 8) 8)"},
    });
}

TEST_CASE("quote")
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

TEST_CASE("do")
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
            )-"},
    });
}

TEST_CASE("let")
{
    testStringsTrue({
            {"let with num", "(= (let [a 37] a) 37)"},

            {"aliasing let", R"-(
            (= (let [x 7]
                 (let [x (inc x)]
                   (assert (= x 8)))
                 x)
               7)
            )-"},
    });
}

// todo: test assert

TEST_CASE("def")
{
    testStringsTrue({
            {"def lookup seq", R"-(
            (def L '(a b c))
            (= L '(a b c))
            )-"},
            {"def loopup num", R"-(
            (= (def M 1) 'M)
            (= M 1)
            )-"},
    });
}

TEST_CASE("count")
{
    // todo: keyword arg (not seq), wrong # args
    testStringsTrue({
            {"vec", "(= (count [:a :b 7 \\a]) 4)"},
            {"list", "(= (count '(:a :b 7 \\a)) 4)"},
            {"map", "(= (count [:a :b 7 \\a]) 4)"},
            {"evaled", "(= (count (when true [:a :b 7 \\a])) 4)"},
    });
}

TEST_CASE("comment")
{
    testStringsTrue({
            {"comment empty", "(= (comment) nil)"},
            {"comment true", "(= (comment true) nil)"},
            {"comment keyword", "(= (comment :a) nil)"},
            {"comment seq", "(= (comment [1 2 3 4]) nil)"},
            {"comment list", "(= (comment (when true 4)) nil)"},
            {"comment is form 1", "(= (if true (comment :b) :a) nil)"},
            {"comment is form 2", "(= (if (comment :b) :a :c) :c)"},
    });
}

TEST_CASE("some")
{
    testStringsTrue({
            // todo: identity function
            {"identity with truthy", "(= (some (fn [x] x) [false 2]) 2)"},
            {"identity with nothing true", "(= (some (fn [x] x) [false false]) nil)"},
            {"identity empty seq", "(= (some (fn [x] x) []) nil)"},
            {"identity eval seq", "(= (some (fn [x] x) ((fn [] [false 3]))) 3)"},
    });

    testStringsLibError({
            {"no args", "(some)"},
            {"one arg", "(some (fn [x] x))"},
            {"not callable", "(some 1 [])"},
            {"not sequence", "(some (fn [x] x) 1)"},
    });
}

TEST_CASE("seq")
{
    testStringsLibError({
            {"no args", "(seq)"},
            {"non sequence arg", "(seq 2)"},
            {"two args", "(seq [] [])"},
    });

    testStringsTrue({
            {"empty sequence", "(= (seq '()) nil)"},
            {"empty vec", "(= (seq []) nil)"},
            {"one item sequence", "(= (seq '(1)) '(1))"},
            {"two item sequence", "(= (seq '(1 2)) '(1 2))"},
            {"two item vec", "(= (seq [1 2]) '(1 2))"},
            // todo: string sequence
            //{"string sequence", "(= (seq \"abc\") '(\\a \\b \\c))"},
            //{"empty string", "(= (seq "") nil)"},
            {"nil", "(= (seq nil) nil)"},
            // todo: map (note: order undefined)
            //{"map", R"-(
            //(= (seq {:key1 \"val1\" :key2 \"val2\"})
            //   '([:key2 \"val2\"] [:key1 \"val1\"]))
            //)-"},
    });
}

TEST_CASE("first")
{
    testStringsLibError({
            {"no args", "(first)"},
            {"non sequence arg", "(first 2)"},
            {"two args", "(first [] [])"},
    });

    testStringsTrue({
            {"empty sequence", "(= (first '()) nil)"},
            {"empty vec", "(= (first []) nil)"},
            {"one item sequence", "(= (first '(1)) 1)"},
            {"two item sequence", "(= (first '(1 2)) 1)"},
            {"two item vec", "(= (first [1 2]) 1)"},
            // todo: string sequence
            //{"string sequence", "(= (first \"abc\") \\a)"},
            //{"empty string", "(= (first "") nil)"},
            {"nil", "(= (first nil) nil)"},
            // todo: map (note: order undefined)
            //{"map", R"-(
            //(= (first {:key1 \"val1\" :key2 \"val2\"})
            //   [:key1 \"val1\"])
            //)-"},
    });
}

TEST_CASE("rest")
{
    testStringsLibError({
            {"no args", "(rest)"},
            {"non sequence arg", "(rest 2)"},
            {"two args", "(rest [] [])"},
    });

    testStringsTrue({
            {"empty sequence", "(= (rest '()) '())"},
            {"empty vec", "(= (rest []) '())"},
            {"one item sequence", "(= (rest '(1)) '())"},
            {"two item sequence", "(= (rest '(1 2)) '(2))"},
            {"two item vec", "(= (rest [1 2]) '(2))"},
            {"three item sequence", "(= (rest '(1 2 3)) '(2 3))"},
            // todo: string sequence
            //{"string sequence", "(= (rest \"abc\") '(\\b \\c))"},
            //{"empty string", "(= (rest "") '())"},
            {"nil", "(= (rest nil) '())"},
            // todo: map (note: order undefined)
            //{"map", R"-(
            //(= (rest {:key1 \"val1\" :key2 \"val2\"})
            //   '([:key2 \"val2\"]))
            //)-"},
    });
}

TEST_CASE("next")
{
    testStringsLibError({
            {"no args", "(next)"},
            {"non sequence arg", "(next 2)"},
            {"two args", "(next [] [])"},
    });

    testStringsTrue({
            {"empty sequence", "(= (next '()) nil)"},
            {"empty vec", "(= (next []) nil)"},
            {"one item sequence", "(= (next '(1)) nil)"},
            {"two item sequence", "(= (next '(1 2)) '(2))"},
            {"two item vec", "(= (next [1 2]) '(2))"},
            {"three item sequence", "(= (next '(1 2 3)) '(2 3))"},
            // todo: string sequence
            //{"string sequence", "(= (next \"abc\") '(\\b \\c))"},
            //{"empty string", "(= (next "") nil)"},
            {"nil", "(= (next nil) nil)"},
            // todo: map (note: order undefined)
            //{"map", R"-(
            //(= (next {:key1 \"val1\" :key2 \"val2\"})
            //   '([:key2 \"val2\"]))
            //)-"},
    });
}

TEST_CASE("cons")
{
    testStringsLibError({
            {"no args", "(cons)"},
            {"one arg", "(cons 2)"},
            {"three args", "(cons 2 [] [])"},
    });

    testStringsTrue({
            {"empty sequence", "(= (cons 2 '()) '(2))"},
            {"empty vec", "(= (cons 2 []) '(2))"},
            {"nil val, nil seq", "(= (cons nil nil) '(nil))"},
            {"nil val, empty seq", "(= (cons nil '()) '(nil))"},
            {"sequence", "(= (cons 1 '(2 3 4 5 6)) '(1 2 3 4 5 6))"},
            {"vec", "(= (cons 1 [2 3 4 5 6]) '(1 2 3 4 5 6))"},
            {"two item sequence", "(= (cons '(3 4) '(1 2)) '((3 4) 1 2))"},
            {"two item vec", "(= (cons [3 4] '(1 2)) '([3 4] 1 2))"},
    });

    // todo: verify that seq arg is not evaluated!
}

TEST_CASE("identity")
{
    testStringsLibError({
            {"no args", "(identity)"},
            {"two args", "(identity 1 2)"},
    });

    testStringsTrue({
            {"int", "(= (identity 4) 4)"},
            {"nil", "(= (identity nil) nil)"},
            {"vec", "(= (identity [1 2 3]) [1 2 3])"},
            {"eval", "(= (identity (+ 2 5)) 7)"},
    });
}

TEST_CASE("reduce")
{
    testStringsLibError({
            {"no args", "(reduce)"},
            {"one arg", "(reduce +)"},
            {"non sequence arg", "(reduce + 2)"},
            {"four args 1", "(reduce + 2 [] 3)"},
            {"four args 2", "(reduce + 2 [] [])"},
    });

    testStringsTrue({
            {"multi item", "(= (reduce + [1 2 3 4 5]) 15)"},
            {"empty seq", "(= (reduce + []) 0)"},
            {"one item", "(= (reduce (fn [_] 3) [1]) 1)"},
            {"two items", "(= (reduce + [1 2]) 3)"},
            {"empty seq with val", "(= (reduce (fn [_] 3) 2 []) 2)"},
            {"two items with val", "(= (reduce + 1 [2 3]) 6)"},
            {"eval seq", "(= (reduce + [(+ 2 3) 1]) 6)"},
            {"eval seq with eval val", "(= (reduce + (inc 1) [(+ 2 3) 1]) 8)"},
    });
}

TEST_CASE("take")
{
    testStringsLibError({
            {"no args", "(take)"},
            // todo: transducer? {"one arg", "(take 1)"},
            {"non sequence arg", "(take 2 2)"},
            {"non numeric arg", "(take [] [])"},
            {"three args", "(take 2 [] [])"},
    });

    testStringsTrue({
            {"subset seq", "(= (take 3 '(1 2 3 4 5 6)) '(1 2 3))"},
            {"subset vector", "(= (take 3 [1 2 3 4 5 6]) '(1 2 3))"},
            {"take more than present", "(= (take 3 [1 2]) '(1 2))"},
            {"one from empty", "(= (take 1 []) '())"},
            {"one from nil", "(= (take 1 nil) '())"},
            {"take zero", "(= (take 0 [1]) '())"},
            {"take negative", "(= (take -1 [1]) '())"},
    });
}

TEST_CASE("iterate")
{
    testStringsLibError({
            {"no args", "(iterate)"},
            {"non call arg", "(iterate 2 2)"},
            {"three args", "(iterate (fn [x] (inc x)) 2 2)"},
    });

    testStringsTrue({
            {"with func", "(= (take 3 (iterate inc 2)) '(3 4 5))"},
            {"with eval func", R"-(
            (= (take 3 (iterate inc (inc 1)))
               '(3 4 5))
            )-"},
            {"with eval arg", R"-(
            (= (take 3 (iterate (identity inc) 2))
               '(3 4 5))
            )-"},
    });
}

TEST_CASE("lazy-seq")
{
    testStringsTrue({
            {"no arg", "(= (lazy-seq) '())"},
            {"func arg", R"-(
            (defn positive-numbers [n]
              (lazy-seq (cons n (positive-numbers (inc n)))))
            (= (take 3 (positive-numbers 3))
               '(3 4 5))
            )-"},
    });
    /* laziness check

; define factory with call to math-stuff up front,
; make sure it makes calls as expected (baseline)
(defn math-stuff [n]
  ; todo: inc call counter
  (* n n))
(defn make-math-stuff-seq [n]
  (cons (math-stuff n) (lazy-seq (math-stuff (inc n)))))
(def math-stuff-seq (make-math-stuff-seq 1))
; todo: verify call count == 1
(take 1 math-stuff-seq)
; todo: verify equals '(1)
; todo: verify call count == 1
(take 1 math-stuff-seq)
; todo: verify equals '(?)
; todo: verify call count == 2
-----
; define factory with lazy-seq call to math-stuff up front
; make sure it makes calls as expected (baseline)
(defn math-stuff [n]
  ; todo: inc call counter
  (* n n))
(defn make-math-stuff-seq [n]
  (lazy-seq (cons (math-stuff n) (make-math-stuff-seq (inc n)))))
(def math-stuff-seq (make-math-stuff-seq 1))
; todo: verify call count == 0
(take 1 math-stuff-seq)
; todo: verify equals '(1)
; todo: verify call count == 1
(take 1 math-stuff-seq)
; todo: verify equals '(?)
; todo: verify call count == 2

; fib:
(defn fib
  ([]
    (fib 1 1))
  ([a b]
    (lazy-seq (cons a (fib b (+ a b))))))
(take 5 (fib))
-> (1 1 2 3 5)
*/
}

TEST_CASE("repeat")
{
    testStringsLibError({
            {"no args", "(repeat)"},
            {"three args", "(repeat 1 2 3)"},
            {"non numeric count", "(repeat :2 2)"},
    });

    testStringsTrue({
            {"forever", R"-(
            (= (take 10 (repeat 8)) '(8 8 8 8 8 8 8 8 8 8))
            )-"},
            {"forever nil", R"-(
            (= (take 3 (repeat nil)) '(nil nil nil))
            )-"},
            {"counted repeats", R"-(
            (= (repeat 10 8) '(8 8 8 8 8 8 8 8 8 8))
            )-"},
            {"zero count", "(= (repeat 0 2) '())"},
            {"negative count", "(= (repeat -2 2) '())"},
            {"multiple call", R"-(
            (def a (repeat 3 3))
            (and (= a '(3 3 3))
                 (= a '(3 3 3)))
            )-"},
    });
}

TEST_CASE("repeatedly")
{
    // todo
}

TEST_CASE("conj")
{
    // todo
}

TEST_CASE("assoc")
{
    // todo
}

TEST_SUITE_END();
