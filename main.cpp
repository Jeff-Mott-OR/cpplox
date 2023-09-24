#include <cstdlib>
#include <iostream>
#include <ostream>
#include <string>
#include <vector>

#include <gsl/gsl>
#include <boost/program_options.hpp>

namespace program_options = boost::program_options;

std::ostream& operator<<(std::ostream& os, std::vector<std::string> vector) {
    for (const auto& string : vector) {
        std::cout << string << ' ';
    }
    return os;
}

int main(int argc, char* argv[]) {
    program_options::options_description options_description {"Usage"};
    options_description.add_options()
        ("help", "Produce help message")
        ("optimization", program_options::value<int>()->default_value(10), "Optimization level")
        ("include-path,I", program_options::value<std::vector<std::string>>(), "Include path")
        ("input-file", program_options::value<std::vector<std::string>>(), "Input file");

    program_options::positional_options_description positional_options_description;
    positional_options_description.add("input-file", -1);

    program_options::variables_map options_map;
    program_options::store(
        program_options::command_line_parser{argc, argv}
            .options(options_description)
            .positional(positional_options_description)
            .run(),
        options_map
    );
    program_options::notify(options_map);

    if (options_map.count("help")) {
        std::cout << options_description << "\n";
        return EXIT_SUCCESS;
    }
    if (options_map.count("include-path")) {
        std::cout << "Include paths are: " << options_map["include-path"].as<std::vector<std::string>>() << "\n";
    }
    if (options_map.count("input-file")) {
        std::cout << "Input files are: " << options_map["input-file"].as<std::vector<std::string>>() << "\n";
    }
    std::cout << "Optimization level is " << options_map["optimization"].as<int>() << "\n";
}
