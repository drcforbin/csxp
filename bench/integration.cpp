#include "env.h"
#include "lib/lib.h"
#include "reader.h"

#include <nanobench.h>

using namespace std::literals;

auto str = R"-(
  (defn f []
    (if (and
          (= (- (inc (inc (inc (* (+ 1 2) 3)))) 1)
             (+ (let [fst (fn [[x y]] x)
                      [x y] ['(1 2 3) '(9 10 11 12)]]
                  (fst y))
                2)
             (- (let [scnd (fn [[x y]] y)
                      [x & y] [9 10 11 12]]
                  (scnd y))
                0))
          (= (let [scnd (fn [[x y]] y)]
               (let [[x :as y] '(9 10 11 12)]
                 (assert (= x 9))
                 (scnd y)))
             10)
          (= (let [[[x1 y1][x2 y2]] [[1 2] [3 4]]]
               [x1 y1 x2 y2])
             [1 2 3 4]))
      (and (= \a \a) (not= \b \a))
      (or false nil)))
  (defn g []
    (if (or
          false nil
          (not (let [s1 "str2" s2 "str2"]
                 (or
                   (= "str1" "str2")
                   (not= "str1" "str1")
                   (= s1 "str2")
                   (not= "str1" s1)
                   (not= "str2" s2)
                   (not= s2 s2))))
          (not true)
          (and
            (= (- 33 24) 9)
            (= (+ 9 0x2) 11)
            (= (+ 10 012) 21)))
      (or (= 46 (* (* 4 3) 4)) false)
      (do
        (assert (not false))
        (not= "string 1" \c))))
  (assert (and (f) (g) (= (f) (g))))
   )-"sv;

void bench_read_run(ankerl::nanobench::Config& cfg)
{
    {
        std::vector<atom::patom> atoms;
        cfg.minEpochIterations(105).run("read logic blob (no run)", [&] {
                                       for (auto val : reader(str, "internal-test"sv)) {
                                           atoms.emplace_back(val);
                                       }
                                   })
                .doNotOptimizeAway(&atoms);
    }
    {
        atom::patom res;
        cfg.minEpochIterations(105).run("read run logic blob", [&] {
                                       // set up basic env
                                       auto env = createEnv();
                                       lib::addCore(env.get());
                                       lib::addMath(env.get());

                                       for (auto val : reader(str, "internal-test"sv)) {
                                           res = env->eval(val);
                                       }
                                   })
                .doNotOptimizeAway(&res);
    }
    {
        std::vector<atom::patom> atoms;
        for (auto val : reader(str, "internal-test"sv)) {
            atoms.emplace_back(val);
        }

        auto env = createEnv();
        lib::addCore(env.get());
        lib::addMath(env.get());

        atom::patom res;
        cfg.minEpochIterations(105).run("run logic blob (no read)", [&] {
                                       for (auto& atom : atoms) {
                                           res = env->eval(atom);
                                       }
                                   })
                .doNotOptimizeAway(&res);
    }
}
