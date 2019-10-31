#define ANKERL_NANOBENCH_IMPLEMENT
#include <nanobench.h>

// todo: better way than extern.

extern void bench_tokenizing(ankerl::nanobench::Config& cfg);
extern void bench_read_run(ankerl::nanobench::Config& cfg);
extern void bench_map_persistent(ankerl::nanobench::Config& cfg);
extern void bench_map_transient(ankerl::nanobench::Config& cfg);
extern void bench_parsing(ankerl::nanobench::Config& cfg);
extern void bench_reading_1(ankerl::nanobench::Config& cfg);
extern void bench_reading_2(ankerl::nanobench::Config& cfg);

int main()
{
    auto cfg = ankerl::nanobench::Config();

    bench_read_run(cfg);
    bench_map_persistent(cfg);
    bench_map_transient(cfg);
    bench_tokenizing(cfg);
    bench_parsing(cfg);
    bench_reading_1(cfg);
    bench_reading_2(cfg);
}
