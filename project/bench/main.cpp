#include <iostream>
#include <string>

// (MSVC) Suppress warnings from dependencies; they're not ours to fix
#pragma warning(push, 0)
    #include <benchmark/benchmark.h>
#pragma warning(pop)
#include <boost/process.hpp>
#include <boost/program_options.hpp>

using std::cout;
using std::string;

namespace process = boost::process;
namespace program_options = boost::program_options;

int main(int argc, /*const*/ char* argv[]) {
    program_options::options_description options_description {
        "Usage: bench [options]\n"
        "\n"
        "Options"
    };
    options_description.add_options()
        ("help", "Print usage information and exit.")
        ("cpplox-file", program_options::value<string>(), "Required. File path to cpplox executable.")
        ("test-scripts-path", program_options::value<string>(), "Required. Path to test scripts.")
        ("jlox-file", program_options::value<string>(), "File path to jlox run cmake script.")
        ("node-file", program_options::value<string>(), "File path to node executable.");

    program_options::variables_map variables_map;
    program_options::store(program_options::parse_command_line(argc, argv, options_description), variables_map);
    program_options::notify(variables_map);

    // Check required options or respond to help
    if (
        variables_map.count("help") ||
        !variables_map.count("cpplox-file") ||
        !variables_map.count("test-scripts-path")
    ) {
        cout << options_description << "\n";
        return 0;
    }

    for (string script_name : {"binary_trees", "equality", "fib", "invocation", "properties", "string_equality"}) {
        benchmark::RegisterBenchmark(("cpplox_" + script_name).c_str(), [script_name, &variables_map] (benchmark::State& state) {
            const auto cpplox = variables_map.at("cpplox-file").as<string>();
            const auto test_script = variables_map.at("test-scripts-path").as<string>() + "/" + script_name + ".lox";
            const auto cmd = "\"" + cpplox + "\" \"" + test_script + "\"";

            for (auto _ : state) {
                process::ipstream cpplox_out;

                // Using boost system rather than std system due to errors on Windows. If the command was unquoted with
                // spaces in the path, then of course I'd get "not a command" errors. But if I wrapped the command in
                // quotes, then I'd get "syntax is incorrect errors". But boost system works just fine either way.
                process::system(cmd, process::std_out > cpplox_out);
            }
        });

        if (variables_map.count("jlox-file") && variables_map.at("jlox-file").as<string>().size()) {
            benchmark::RegisterBenchmark(("jlox_" + script_name).c_str(), [script_name, &variables_map] (benchmark::State& state) {
                const auto jlox = variables_map.at("jlox-file").as<string>();
                const auto test_script = variables_map.at("test-scripts-path").as<string>() + "/" + script_name + ".lox";
                const auto cmd = "cmake -P \"" + jlox + "\" \"" + test_script + "\"";

                for (auto _ : state) {
                    process::ipstream jlox_out;
                    process::system(cmd, process::std_out > jlox_out);
                }
            });
        }

        if (variables_map.count("node-file") && variables_map.at("node-file").as<string>().size()) {
            benchmark::RegisterBenchmark(("node_" + script_name).c_str(), [script_name, &variables_map] (benchmark::State& state) {
                const auto node = variables_map.at("node-file").as<string>();
                const auto test_script = variables_map.at("test-scripts-path").as<string>() + "/" + script_name + ".js";
                const auto cmd = "\"" + node + "\" \"" + test_script + "\"";

                for (auto _ : state) {
                    process::ipstream node_out;
                    process::system(cmd, process::std_out > node_out);
                }
            });
        }
    }

    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
}
