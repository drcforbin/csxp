#include "reader.h"

#include <nanobench.h>

extern const std::string_view arbitrary_clj;
extern const std::string_view arbitrary_long_clj;

void bench_reading_1(ankerl::nanobench::Config& cfg)
{
    atom::patom res;
    cfg.minEpochIterations(105).run("read blob 1", [&] {
                                   auto r = createReader(arbitrary_clj, "internal-test");

                                   while (r->next()) {
                                       res = r->value();
                                   }
                               })
            .doNotOptimizeAway(&res);

    res.reset();
    cfg.minEpochIterations(10).run("read long blob 1", [&] {
                                  auto r = createReader(arbitrary_long_clj, "internal-test");

                                  while (r->next()) {
                                      res = r->value();
                                  }
                              })
            .doNotOptimizeAway(&res);
}

void bench_reading_2(ankerl::nanobench::Config& cfg)
{
    atom::patom res;
    cfg.minEpochIterations(105).run("read blob 2", [&] {
                                   auto r = createReader2(arbitrary_clj, "internal-test");

                                   while (r->next()) {
                                       res = r->value();
                                   }
                               })
            .doNotOptimizeAway(&res);

    res.reset();
    cfg.minEpochIterations(105).run("read blob 2 (it)", [&] {
                                   auto r = createReader2(arbitrary_clj, "internal-test");

           for (auto val : *r) {
               res = val;
           }
                               })
            .doNotOptimizeAway(&res);

    res.reset();
    cfg.minEpochIterations(105).run("read blob 2 (it 2)", [&] {
           for (auto val : reader(arbitrary_clj, "internal-test")) {
               res = val;
           }
                               })
            .doNotOptimizeAway(&res);

    res.reset();
    cfg.minEpochIterations(10).run("read long blob 2", [&] {
                                  auto r = createReader2(arbitrary_long_clj, "internal-test");

                                  while (r->next()) {
                                      res = r->value();
                                  }
                              })
            .doNotOptimizeAway(&res);

    res.reset();
    cfg.minEpochIterations(10).run("read long blob 2 (it)", [&] {
                                  auto r = createReader2(arbitrary_long_clj, "internal-test");

           for (auto val : *r) {
               res = val;
           }
                              })
            .doNotOptimizeAway(&res);

    res.reset();
    cfg.minEpochIterations(10).run("read long blob 2 (it 2)", [&] {
           for (auto val : reader(arbitrary_long_clj, "internal-test")) {
               res = val;
           }
                              })
            .doNotOptimizeAway(&res);
}
