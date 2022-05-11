#include <fstream>
#include <iostream>

#include "lox.hpp"

namespace {
    void run_prompt(motts::lox::Lox& lox, bool debug = false) {
        while (true) {
            std::cout << "> ";

            std::string source_line;
            std::getline(std::cin, source_line);

            // If the user makes a mistake, it shouldn't kill their entire session
            try {
                lox.vm.run(lox.compile(source_line), debug);
            } catch (const std::exception& error) {
                std::cerr << error.what() << '\n';
            }
        }
    }

    void run_file(motts::lox::Lox& lox, const char* path, bool debug = false) {
        const auto source = ([&] () {
            std::ifstream in {path};
            in.exceptions(std::ifstream::failbit | std::ifstream::badbit);
            return std::string{std::istreambuf_iterator<char>{in}, std::istreambuf_iterator<char>{}};
        })();

        lox.vm.run(lox.compile(source), debug);
    }
}

int main(int argc, const char* argv[]) {
    motts::lox::Lox lox;

    try {
        // STL-like container interface to argv
        gsl::span<const char*> argv_span {argv, argc};

        if (argv_span.size() == 1) {
            run_prompt(lox);
        } else if (argv_span.size() == 2 && gsl::cstring_span<>{gsl::ensure_z(argv_span.at(1), 256)} == "-DEBUG") {
            run_prompt(lox, true);
        } else if (argv_span.size() == 2) {
            run_file(lox, argv_span.at(1));
        } else if (argv_span.size() == 3 && gsl::cstring_span<>{gsl::ensure_z(argv_span.at(1), 256)} == "-DEBUG") {
            run_file(lox, argv_span.at(2), true);
        } else {
            std::cerr << "Usage: cpploxbc [-DEBUG] [path]\n";
            exit(EXIT_FAILURE);
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        exit(EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
}
