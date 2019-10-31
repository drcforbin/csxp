#include "tokenizer.h"

#include <nanobench.h>

extern const std::string_view arbitrary_clj;
extern const std::string_view arbitrary_long_clj;

void bench_tokenizing(ankerl::nanobench::Config& cfg)
{
    int count = 0;
    cfg.minEpochIterations(100).run("tokenize blob", [&] {
           auto tz = createTokenizer(arbitrary_clj, "internal-test");

           while (tz->next()) {
               if (tz->value().t != Tk::none) {
                   count++;
               }
           }
       }).doNotOptimizeAway(&count);

    count = 0;
    cfg.minEpochIterations(100).run("tokenize blob (it)", [&] {
           auto tz = createTokenizer(arbitrary_clj, "internal-test");

           for (auto val : *tz) {
               if (val.t != Tk::none) {
                   count++;
               }
           }
       }).doNotOptimizeAway(&count);

    count = 0;
    cfg.run("tokenize long blob", [&] {
           auto tz = createTokenizer(arbitrary_long_clj, "internal-test");

           while (tz->next()) {
               if (tz->value().t != Tk::none) {
                   count++;
               }
           }
       }).doNotOptimizeAway(&count);

    count = 0;
    cfg.run("tokenize long blob (it)", [&] {
           auto tz = createTokenizer(arbitrary_long_clj, "internal-test");

           for (auto val : *tz) {
               if (val.t != Tk::none) {
                   count++;
               }
           }
       }).doNotOptimizeAway(&count);
}
