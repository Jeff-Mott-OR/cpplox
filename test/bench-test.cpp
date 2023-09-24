#include <sstream>

#include <benchmark/benchmark.h>
#include <boost/process.hpp>

#include "../src/lox.hpp"

static void bench_run_file(benchmark::State& state)
{
    for (auto _ : state) {
        std::ostringstream os;
        motts::lox::Lox lox {os};
        run_file(lox, "../src/test/lox/hello.lox");
    }
}
BENCHMARK(bench_run_file);

static void bench_run_file_spawn_process(benchmark::State& state)
{
    for (auto _ : state) {
        boost::process::ipstream cpplox_out;
        boost::process::ipstream cpplox_err;
        boost::process::system(
            "cpploxbc ../src/test/lox/hello.lox",
            boost::process::std_out > cpplox_out,
            boost::process::std_err > cpplox_err
        );
    }
}
BENCHMARK(bench_run_file_spawn_process);

BENCHMARK_MAIN();
