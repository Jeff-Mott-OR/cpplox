#include <sstream>

#include <benchmark/benchmark.h>

#include "../src/lox.hpp"

static void bench_binary_trees(benchmark::State& state)
{
    for (auto _ : state) {
        std::ostringstream os;
        motts::lox::Lox lox{os};
        run_file(lox, "../src/test/lox/bench/binary_trees.lox");
    }
}

BENCHMARK(bench_binary_trees);

static void bench_equality(benchmark::State& state)
{
    for (auto _ : state) {
        std::ostringstream os;
        motts::lox::Lox lox{os};
        run_file(lox, "../src/test/lox/bench/equality.lox");
    }
}

BENCHMARK(bench_equality);

static void bench_fib(benchmark::State& state)
{
    for (auto _ : state) {
        std::ostringstream os;
        motts::lox::Lox lox{os};
        run_file(lox, "../src/test/lox/bench/fib.lox");
    }
}

BENCHMARK(bench_fib);

static void bench_invocation(benchmark::State& state)
{
    for (auto _ : state) {
        std::ostringstream os;
        motts::lox::Lox lox{os};
        run_file(lox, "../src/test/lox/bench/invocation.lox");
    }
}

BENCHMARK(bench_invocation);

static void bench_properties(benchmark::State& state)
{
    for (auto _ : state) {
        std::ostringstream os;
        motts::lox::Lox lox{os};
        run_file(lox, "../src/test/lox/bench/properties.lox");
    }
}

BENCHMARK(bench_properties);

static void bench_string_equality(benchmark::State& state)
{
    for (auto _ : state) {
        std::ostringstream os;
        motts::lox::Lox lox{os};
        run_file(lox, "../src/test/lox/bench/string_equality.lox");
    }
}

BENCHMARK(bench_string_equality);

static void bench_trees(benchmark::State& state)
{
    for (auto _ : state) {
        std::ostringstream os;
        motts::lox::Lox lox{os};
        run_file(lox, "../src/test/lox/bench/trees.lox");
    }
}

BENCHMARK(bench_trees);

static void bench_zoo(benchmark::State& state)
{
    for (auto _ : state) {
        std::ostringstream os;
        motts::lox::Lox lox{os};
        run_file(lox, "../src/test/lox/bench/zoo.lox");
    }
}

BENCHMARK(bench_zoo);

static void bench_zoo_batch(benchmark::State& state)
{
    for (auto _ : state) {
        std::ostringstream os;
        motts::lox::Lox lox{os};
        run_file(lox, "../src/test/lox/bench/zoo_batch.lox");
    }
}

BENCHMARK(bench_zoo_batch);

BENCHMARK_MAIN();
