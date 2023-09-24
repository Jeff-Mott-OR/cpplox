#include <benchmark/benchmark.h>
#include <boost/process.hpp>

#include "../src/lox.hpp"

namespace process = boost::process;

static void bench_run_file(benchmark::State& state) {
    // Perform setup here

    for (auto _ : state) {
        // This code gets timed
        std::ostringstream os;
        motts::lox::run_file("../test/lox/hello.lox", os, /* debug = */ false);
    }
}
// Register the function as a benchmark
BENCHMARK(bench_run_file);

static void bench_run_file_spawn_process(benchmark::State& state) {
    for (auto _ : state) {
        process::ipstream cpplox_out;
        process::ipstream cpplox_err;
        process::system(
            "cpploxbc ../test/lox/hello.lox",
            process::std_out > cpplox_out,
            process::std_err > cpplox_err
        );
    }
}
BENCHMARK(bench_run_file_spawn_process);

// Run the benchmark
BENCHMARK_MAIN();
