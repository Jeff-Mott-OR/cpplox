#include "lox.hpp"

#include <deque>
#include <fstream>
#include <iterator>
#include <stdexcept>

#include "compiler.hpp"

namespace motts::lox
{
    Lox::Lox(std::ostream& cout_arg, std::ostream& cerr_arg, std::istream& cin_arg, bool debug_arg)
        : debug{debug_arg},
          cout{cout_arg},
          cerr{cerr_arg},
          cin{cin_arg}
    {
    }

    void run_file(Lox& lox, const std::string& file_path)
    {
        // Immediately invoked lambdas to encapsulate initialization of const variables and to dispose objects the RAII way.
        const auto bytecode = [&] {
            const auto source = [&] {
                std::ifstream file_stream{file_path};
                file_stream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
                return std::string{std::istreambuf_iterator<char>{file_stream}, {}};
            }();

            // The compiler is expected to make owning copies of any source fragments it needs.
            return compile(lox.gc_heap, lox.interned_strings, source);
        }();

        lox.vm.run(bytecode);
    }

    void run_prompt(Lox& lox)
    {
        while (true) {
            lox.cout << "> ";

            std::string source_line;
            std::getline(lox.cin, source_line);

            try {
                const auto bytecode = compile(lox.gc_heap, lox.interned_strings, source_line);
                lox.vm.run(bytecode);
            } catch (const std::runtime_error& error) {
                // If the user makes a mistake, it shouldn't kill their entire session.
                lox.cerr << error.what() << '\n';
            }
        }
    }
}
