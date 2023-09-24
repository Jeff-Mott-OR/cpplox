#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

#include <boost/program_options.hpp>

#include "compiler.hpp"
#include "vm.hpp"

namespace program_options = boost::program_options;

namespace {
    using namespace motts::lox;

    void run_file(const std::string& input_file, bool debug) {
        const auto source = ([&] () {
            std::ifstream in {input_file};
            in.exceptions(std::ifstream::failbit | std::ifstream::badbit);
            return std::string{std::istreambuf_iterator<char>{in}, std::istreambuf_iterator<char>{}};
        })();

        const auto chunk = compile(source);
        run(chunk, std::cout, debug);
    }

    void run_prompt(bool debug) {
        VM vm;

        while (true) {
            std::cout << "> ";

            std::string source_line;
            std::getline(std::cin, source_line);

            // If the user makes a mistake, it shouldn't kill their entire session
            try {
                const auto chunk = compile(source_line);
                vm.run(chunk, std::cout, debug);
            } catch (const std::exception& error) {
                std::cerr << error.what() << "\n";
            }
        }
    }
}

int main(int argc, char* argv[]) {
    program_options::options_description options {"Usage"};
    options.add_options()
        ("help", "Show help message")
        ("debug", program_options::bool_switch(), "Disassemble instructions")
        ("input-file", program_options::value<std::string>(), "Input file");

    program_options::positional_options_description positional_options;
    positional_options.add("input-file", 1);

    program_options::variables_map options_map;
    program_options::store(
        program_options::command_line_parser{argc, argv}
            .options(options)
            .positional(positional_options)
            .run(),
        options_map
    );

    if (options_map.count("help")) {
        std::cout << options << "\n";
        return EXIT_SUCCESS;
    }

    try {
        if (options_map.count("input-file")) {
            run_file(options_map["input-file"].as<std::string>(), options_map["debug"].as<bool>());
        } else {
            run_prompt(options_map["debug"].as<bool>());
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << "\n";
        return EXIT_FAILURE;
    }
}
