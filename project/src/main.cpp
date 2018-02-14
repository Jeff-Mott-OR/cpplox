#include <cstdlib>

#include <exception>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

#include <gsl/span>

#include "exception.hpp"
#include "interpreter.hpp"
#include "parser.hpp"
#include "scanner.hpp"

using std::cin;
using std::cout;
using std::exception;
using std::exit;
using std::getline;
using std::ifstream;
using std::istreambuf_iterator;
using std::string;

using gsl::span;

using motts::lox::Interpreter;
using motts::lox::parse;
using motts::lox::Runtime_error;
using motts::lox::Token_iterator;

auto run(const string& source) {
    const auto expression = parse(Token_iterator{source});

    Interpreter interpreter;
    expression->accept(interpreter);
    cout << interpreter.result() << "\n";
}

auto run_file(const string& path) {
    // IIFE to limit scope of ifstream
    const auto source = ([&] () {
        ifstream in {path};
        in.exceptions(ifstream::failbit | ifstream::badbit);
        const string source {istreambuf_iterator<char>{in}, istreambuf_iterator<char>{}};

        return source;
    })();

    run(source);
}

auto run_prompt() {
    while (true) {
        cout << "> ";

        string source_line;
        getline(cin, source_line);

        // If the user makes a mistake, it shouldn't kill their entire session
        try {
            run(source_line);
        } catch (const Runtime_error& e) {
            cout << e.what() << "\n";
        }
    }
}

int main(int argc, const char* argv[]) {
    try {
        // STL container-like interface to argv
        span<const char*> argv_span {argv, argc};

        if (argv_span.size() > 2) {
            cout << "Usage: cpplox [script]\n";
        } else if (argv_span.size() == 2) {
            run_file(argv_span.at(1));
        } else {
            run_prompt();
        }
    } catch (const exception& e) {
        cout << "Something went wrong: " << e.what() << "\n";

        exit(EXIT_FAILURE);
    } catch (...) {
        cout << "Something went wrong.\n";

        exit(EXIT_FAILURE);
    }
}
