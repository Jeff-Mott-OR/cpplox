#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

#include <boost/program_options.hpp>

#include "lox.hpp"

namespace program_options = boost::program_options;

int main(int argc, char* argv[]) {
    program_options::options_description options {"Usage"};
    options.add_options()
        ("help", "Show help message")
        ("debug", "Disassemble instructions")
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
        std::cout << options << '\n';
        return EXIT_SUCCESS;
    }

    try {
        if (options_map.count("input-file")) {
            motts::lox::run_file(options_map["input-file"].as<std::string>(), std::cout, options_map.count("debug"));
        } else {
            motts::lox::run_prompt(options_map.count("debug"));
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
