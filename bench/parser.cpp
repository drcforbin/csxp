#include "parser.h"

#include <nanobench.h>

extern const std::string_view arbitrary_clj;
extern const std::string_view arbitrary_long_clj;

void bench_parsing(ankerl::nanobench::Config& cfg)
{
    atom::patom res;
    cfg.run("parse blob", [&] {
           auto tz = createTokenizer(arbitrary_clj, "internal-test");
           auto p = createParser(tz);

           while (p->next()) {
               res = p->value();
           }
       }).doNotOptimizeAway(&res);

    res.reset();
    cfg.run("parse blob (it)", [&] {
           auto tz = createTokenizer(arbitrary_clj, "internal-test");
           auto p = createParser(tz);

           for (auto val : *p) {
               res = val;
           }
       }).doNotOptimizeAway(&res);

    res.reset();
    cfg.run("parse long blob", [&] {
           auto tz = createTokenizer(arbitrary_long_clj, "internal-test");
           auto p = createParser(tz);

           while (p->next()) {
               res = p->value();
           }
       }).doNotOptimizeAway(&res);

    res.reset();
    cfg.run("parse long blob (it)", [&] {
           auto tz = createTokenizer(arbitrary_long_clj, "internal-test");
           auto p = createParser(tz);

           for (auto val : *p) {
               res = val;
           }
       }).doNotOptimizeAway(&res);
}
