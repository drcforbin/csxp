#define ANKERL_NANOBENCH_IMPLEMENT
#include <nanobench.h>

#include "csxp/logging.h"

// todo: better way than extern.

extern void bench_read_run(ankerl::nanobench::Config& cfg);
extern void bench_map_persistent(ankerl::nanobench::Config& cfg);
extern void bench_map_transient(ankerl::nanobench::Config& cfg);
extern void bench_reading(ankerl::nanobench::Config& cfg);

int main()
{
    logging::get("env")->level(logging::off);

    auto cfg = ankerl::nanobench::Config();

    bench_read_run(cfg);
    bench_map_persistent(cfg);
    bench_map_transient(cfg);
    bench_reading(cfg);
}
