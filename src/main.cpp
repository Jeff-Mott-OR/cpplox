#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

#include <boost/program_options.hpp>

#include "lox.hpp"

int main(int argc, const char* argv[])
{
    boost::program_options::options_description options;

    // clang-format off
    options.add_options()
        ("help", "Show this help message.")
        ("input-file", boost::program_options::value<std::string>(), "Lox script file to run.")
        ("debug", "Disassemble instructions and dump the stack.");
    // clang-format on

    boost::program_options::positional_options_description positional_options;
    positional_options.add("input-file", -1);

    boost::program_options::variables_map options_map;

    // clang-format off
    boost::program_options::store(
        boost::program_options::command_line_parser{argc, argv}
            .options(options)
            .positional(positional_options)
            .run(),
        options_map
    );
    // clang-format on

    if (options_map.contains("help")) {
        std::cout << options << '\n';
        return EXIT_SUCCESS;
    }

    try {
        motts::lox::Lox lox{std::cout, std::cerr, std::cin, options_map.contains("debug")};

        if (options_map.contains("input-file")) {
            run_file(lox, options_map["input-file"].as<std::string>());
        } else {
            run_prompt(lox);
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
