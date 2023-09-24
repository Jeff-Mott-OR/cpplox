#include "lox.hpp"

#include <deque>
#include <fstream>
#include <iterator>
#include <stdexcept>

#include "compiler.hpp"

namespace motts { namespace lox
{
    Lox::Lox(std::ostream& cout_arg, std::ostream& cerr_arg, std::istream& cin_arg, bool debug_arg)
        : debug {debug_arg},
          cout {cout_arg},
          cerr {cerr_arg},
          cin {cin_arg}
    {
    }

    void run_file(Lox& lox, const std::string& file_path)
    {
        // Immediately invoked lambda to encapsulate initialization of a const variable, and to dispose the ifstream the RAII way.
        const auto source = [&] {
            std::ifstream file_stream {file_path};
            file_stream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
            return std::string{std::istreambuf_iterator<char>{file_stream}, {}};
        }();

        const auto bytecode = compile(lox.gc_heap, source);
        lox.vm.run(bytecode);
    }

    void run_prompt(Lox& lox)
    {
        // The VM keeps owning references to bytecode, and the bytecode keeps non-owning views to the source text.
        // That means source character buffers need to stay allocated and have a stable address.
        std::deque<std::string> source_lines_owner;

        while (true) {
            lox.cout << "> ";

            source_lines_owner.push_back({});
            auto& new_source_line = source_lines_owner.back();
            std::getline(lox.cin, new_source_line);

            try {
                const auto bytecode = compile(lox.gc_heap, new_source_line);
                lox.vm.run(bytecode);
            } catch (const std::exception& error) {
                // If the user makes a mistake, it shouldn't kill their entire session.
                lox.cerr << error.what() << '\n';
            }
        }
    }
}}
