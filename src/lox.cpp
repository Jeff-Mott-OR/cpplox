#include "lox.hpp"

#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>

#include "compiler.hpp"
#include "vm.hpp"

namespace motts { namespace lox {
    void run_file(const std::string& input_file, std::ostream& os, bool debug) {
        const auto source = ([&] () {
            std::ifstream in {input_file};
            in.exceptions(std::ifstream::failbit | std::ifstream::badbit);
            return std::string{std::istreambuf_iterator<char>{in}, std::istreambuf_iterator<char>{}};
        })();

        const auto chunk = compile(source);
        run(chunk, os, debug);
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
}}
