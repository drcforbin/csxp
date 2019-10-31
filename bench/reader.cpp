#include "reader.h"

#include <nanobench.h>

extern const std::string_view arbitrary_clj;
extern const std::string_view arbitrary_long_clj;

void bench_reading(ankerl::nanobench::Config& cfg)
{
    atom::patom res;
    cfg.minEpochIterations(105).run(
                                       "read blob", [&] {
                                           auto r = createReader(arbitrary_clj, "internal-test");
                                           while (r->next()) {
                                               res = r->value();
                                           }
                                       })
            .doNotOptimizeAway(&res);

    res.reset();
    cfg.minEpochIterations(105).run(
                                       "read blob (it)", [&] {
                                           for (auto val : reader(arbitrary_clj, "internal-test")) {
                                               res = val;
                                           }
                                       })
            .doNotOptimizeAway(&res);

    res.reset();
    cfg.minEpochIterations(10).run(
                                      "read long blob", [&] {
                                          auto r = createReader(arbitrary_long_clj, "internal-test");
                                          while (r->next()) {
                                              res = r->value();
                                          }
                                      })
            .doNotOptimizeAway(&res);

    res.reset();
    cfg.minEpochIterations(10).run(
                                      "read long blob (it)", [&] {
                                          for (auto val : reader(arbitrary_long_clj, "internal-test")) {
                                              res = val;
                                          }
                                      })
            .doNotOptimizeAway(&res);
}
