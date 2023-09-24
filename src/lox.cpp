#include "lox.hpp"

#include <fstream>
#include <iterator>
#include <stdexcept>

#include "compiler.hpp"

namespace motts { namespace lox {
    Lox::Lox(std::ostream& cout_arg, std::ostream& cerr_arg, std::istream& cin_arg, bool debug_arg)
        : debug {debug_arg},
          cout {cout_arg},
          cerr {cerr_arg},
          cin {cin_arg}
    {}

    void run_file(Lox& lox, const std::string& file_path) {
        const auto source = [&] {
            std::ifstream file_stream {file_path};
            file_stream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
            return std::string{std::istreambuf_iterator<char>{file_stream}, std::istreambuf_iterator<char>{}};
        }();

        const auto chunk = compile(lox.gc_heap, source);
        lox.vm.run(chunk);
    }

    void run_prompt(Lox& lox) {
        std::vector<std::string> source_lines_owner;

        while (true) {
            lox.cout << "> ";

            source_lines_owner.push_back({});
            auto& source_line = source_lines_owner.back();
            std::getline(lox.cin, source_line);

            // If the user makes a mistake, it shouldn't kill their entire session.
            try {
                const auto chunk = compile(lox.gc_heap, source_line);
                lox.vm.run(chunk);
            } catch (const std::exception& error) {
                lox.cerr << error.what() << '\n';
            }
        }
    }
}}
