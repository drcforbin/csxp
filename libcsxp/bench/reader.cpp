#include "csxp/reader.h"
#include "nanobench.h"

extern const std::string_view arbitrary_clj;
extern const std::string_view arbitrary_long_clj;

void bench_reading(ankerl::nanobench::Config& cfg)
{
    csxp::patom res;
    cfg.minEpochIterations(105).run(
                                       "read blob", [&] {
                                           for (auto val : csxp::reader(arbitrary_clj, "internal-test")) {
                                               res = val;
                                           }
                                       })
            .doNotOptimizeAway(&res);

    res.reset();
    cfg.minEpochIterations(10).run(
                                      "read long blob", [&] {
                                          for (auto val : csxp::reader(arbitrary_long_clj, "internal-test")) {
                                              res = val;
                                          }
                                      })
            .doNotOptimizeAway(&res);
}
