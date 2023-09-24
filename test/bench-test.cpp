#include <sstream>

#include <benchmark/benchmark.h>
#include <boost/process.hpp>

#include "../src/lox.hpp"

static void bench_static_lib_run_empty_file(benchmark::State& state)
{
    for (auto _ : state) {
        std::ostringstream os;
        motts::lox::Lox lox {os};
        run_file(lox, "../src/test/lox/empty_file.lox");
    }
}
BENCHMARK(bench_static_lib_run_empty_file);

#define MOTTS_LOX_MAKE_SPAWN_PROCESS_BENCH(TEST_NAME, EXECUTABLE, TEST_FILE) \
    static void TEST_NAME(benchmark::State& state) \
    { \
        for (auto _ : state) { \
            boost::process::ipstream out; \
            boost::process::ipstream err; \
            boost::process::system( \
                EXECUTABLE " ../src/test/lox/" TEST_FILE, \
                boost::process::std_out > out, \
                boost::process::std_err > err \
            ); \
        } \
    } \
    BENCHMARK(TEST_NAME);

MOTTS_LOX_MAKE_SPAWN_PROCESS_BENCH(bench_empty_file_cpploxbc, "cpploxbc", "empty_file.lox")

MOTTS_LOX_MAKE_SPAWN_PROCESS_BENCH(bench_binary_trees_cpploxbc, "cpploxbc", "bench/binary_trees.lox")
MOTTS_LOX_MAKE_SPAWN_PROCESS_BENCH(bench_binary_trees_jlox, "java -cp _deps/crafting_interpreters-src/build/java com.craftinginterpreters.lox.Lox", "bench/binary_trees.lox")
MOTTS_LOX_MAKE_SPAWN_PROCESS_BENCH(bench_binary_trees_clox, "_deps/crafting_interpreters-src/build/clox", "bench/binary_trees.lox")
MOTTS_LOX_MAKE_SPAWN_PROCESS_BENCH(bench_binary_trees_node, "node", "bench/binary_trees.js")

MOTTS_LOX_MAKE_SPAWN_PROCESS_BENCH(bench_equality_cpploxbc, "cpploxbc", "bench/equality.lox")
MOTTS_LOX_MAKE_SPAWN_PROCESS_BENCH(bench_equality_jlox, "java -cp _deps/crafting_interpreters-src/build/java com.craftinginterpreters.lox.Lox", "bench/equality.lox")
MOTTS_LOX_MAKE_SPAWN_PROCESS_BENCH(bench_equality_clox, "_deps/crafting_interpreters-src/build/clox", "bench/equality.lox")
MOTTS_LOX_MAKE_SPAWN_PROCESS_BENCH(bench_equality_node, "node", "bench/equality.js")

MOTTS_LOX_MAKE_SPAWN_PROCESS_BENCH(bench_fib_cpploxbc, "cpploxbc", "bench/fib.lox")
MOTTS_LOX_MAKE_SPAWN_PROCESS_BENCH(bench_fib_jlox, "java -cp _deps/crafting_interpreters-src/build/java com.craftinginterpreters.lox.Lox", "bench/fib.lox")
MOTTS_LOX_MAKE_SPAWN_PROCESS_BENCH(bench_fib_clox, "_deps/crafting_interpreters-src/build/clox", "bench/fib.lox")
MOTTS_LOX_MAKE_SPAWN_PROCESS_BENCH(bench_fib_node, "node", "bench/fib.js")

MOTTS_LOX_MAKE_SPAWN_PROCESS_BENCH(bench_invocation_cpploxbc, "cpploxbc", "bench/invocation.lox")
MOTTS_LOX_MAKE_SPAWN_PROCESS_BENCH(bench_invocation_jlox, "java -cp _deps/crafting_interpreters-src/build/java com.craftinginterpreters.lox.Lox", "bench/invocation.lox")
MOTTS_LOX_MAKE_SPAWN_PROCESS_BENCH(bench_invocation_clox, "_deps/crafting_interpreters-src/build/clox", "bench/invocation.lox")
MOTTS_LOX_MAKE_SPAWN_PROCESS_BENCH(bench_invocation_node, "node", "bench/invocation.js")

MOTTS_LOX_MAKE_SPAWN_PROCESS_BENCH(bench_properties_cpploxbc, "cpploxbc", "bench/properties.lox")
MOTTS_LOX_MAKE_SPAWN_PROCESS_BENCH(bench_properties_jlox, "java -cp _deps/crafting_interpreters-src/build/java com.craftinginterpreters.lox.Lox", "bench/properties.lox")
MOTTS_LOX_MAKE_SPAWN_PROCESS_BENCH(bench_properties_clox, "_deps/crafting_interpreters-src/build/clox", "bench/properties.lox")
MOTTS_LOX_MAKE_SPAWN_PROCESS_BENCH(bench_properties_node, "node", "bench/properties.js")

MOTTS_LOX_MAKE_SPAWN_PROCESS_BENCH(bench_string_equality_cpploxbc, "cpploxbc", "bench/string_equality.lox")
MOTTS_LOX_MAKE_SPAWN_PROCESS_BENCH(bench_string_equality_jlox, "java -cp _deps/crafting_interpreters-src/build/java com.craftinginterpreters.lox.Lox", "bench/string_equality.lox")
MOTTS_LOX_MAKE_SPAWN_PROCESS_BENCH(bench_string_equality_clox, "_deps/crafting_interpreters-src/build/clox", "bench/string_equality.lox")
MOTTS_LOX_MAKE_SPAWN_PROCESS_BENCH(bench_string_equality_node, "node", "bench/string_equality.js")

BENCHMARK_MAIN();
