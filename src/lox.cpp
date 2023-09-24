#include "lox.hpp"

#include <fstream>
#include <iterator>
#include <stdexcept>

#include "compiler.hpp"

namespace motts { namespace lox {
    Lox::Lox(std::ostream& os_arg, bool debug_arg)
        : os {os_arg},
          debug {debug_arg}
    {}

    Chunk Lox::compile(std::string_view source) {
        return motts::lox::compile(gc_heap, source);
    }

    void Lox::run_file(const std::string& input_file) {
        const auto source = [&] {
            std::ifstream in {input_file};
            in.exceptions(std::ifstream::failbit | std::ifstream::badbit);
            return std::string{std::istreambuf_iterator<char>{in}, std::istreambuf_iterator<char>{}};
        }();

        const auto chunk = compile(source);
        vm.run(chunk);
    }

    void Lox::run_prompt() {
        std::vector<std::string> source_lines_owner;

        while (true) {
            std::cout << "> ";

            source_lines_owner.push_back({});
            auto& source_line = source_lines_owner.back();
            std::getline(std::cin, source_line);

            // If the user makes a mistake, it shouldn't kill their entire session
            try {
                const auto chunk = compile(source_line);
                vm.run(chunk);
            } catch (const std::exception& error) {
                std::cerr << error.what() << "\n";
            }
        }
    }
}}
