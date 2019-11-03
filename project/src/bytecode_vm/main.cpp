#include <cstdlib>

#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

#include <gsl/span>

#include "vm.hpp"

using std::cin;
using std::cout;
using std::exit;
using std::getline;
using std::ifstream;
using std::istreambuf_iterator;
using std::string;

using gsl::span;

namespace loxns = motts::lox;

// Not exported (internal linkage)
namespace {
    void repl(loxns::VM& vm) {
        while (true) {
            cout << "> ";

            string source_line;
            getline(cin, source_line);

            vm.interpret(source_line);
        }
    }

    void run_file(loxns::VM& vm, const string& path) {
        // IIFE to limit scope of ifstream
        const auto source = ([&] () {
            ifstream in {path};
            in.exceptions(ifstream::failbit | ifstream::badbit);
            return string{istreambuf_iterator<char>{in}, istreambuf_iterator<char>{}};
        })();

        vm.interpret(source);
    }
}

int main(int argc, const char* argv[]) {
    loxns::VM vm;

    // STL-like container interface to argv
    span<const char*> argv_span {argv, argc};

    if (argv_span.size() == 1) {
        repl(vm);
    } else if (argv_span.size() == 2) {
        run_file(vm, argv_span.at(1));
    } else {
        cout << "Usage: cpploxbc [path]\n";
        exit(EXIT_FAILURE);
    }
}
