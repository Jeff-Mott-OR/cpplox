// Related header
// C standard headers
#include <cstdlib>
// C++ standard headers
#include <exception>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>
// Third-party headers
#include <boost/algorithm/string/join.hpp>
#include <gsl/span>
// This project's headers
#include "exception.hpp"
#include "interpreter.hpp"
#include "parser.hpp"
#include "resolver.hpp"
#include "scanner.hpp"

using std::cerr;
using std::cin;
using std::cout;
using std::exception;
using std::exit;
using std::getline;
using std::ifstream;
using std::istreambuf_iterator;
using std::string;
using std::vector;

using boost::algorithm::join;
using gsl::span;

namespace lox = motts::lox;

auto run(const string& source, lox::Interpreter& interpreter) {
    const auto statements = lox::parse(lox::Token_iterator{source});

    lox::Resolver resolver {interpreter};
    vector<string> resolver_errors;
    for (const auto& statement : statements) {
        try {
            statement->accept(statement, resolver);
        } catch (const lox::Resolver_error& error) {
            resolver_errors.push_back(error.what());
        }
    }
    if (resolver_errors.size()) {
        throw lox::Resolver_error{join(resolver_errors, "\n")};
    }

    for (const auto& statement : statements) {
        statement->accept(statement, interpreter);
    }
}

auto run(const string& source) {
    lox::Interpreter interpreter;
    run(source, interpreter);
}

auto run_file(const string& path) {
    // IIFE to limit scope of ifstream
    const auto source = ([&] () {
        ifstream in {path};
        in.exceptions(ifstream::failbit | ifstream::badbit);
        return string{istreambuf_iterator<char>{in}, istreambuf_iterator<char>{}};
    })();

    run(source);
}

auto run_prompt() {
    lox::Interpreter interpreter;

    while (true) {
        cout << "> ";

        string source_line;
        getline(cin, source_line);

        // If the user makes a mistake, it shouldn't kill their entire session
        try {
            run(source_line, interpreter);
        } catch (const lox::Runtime_error& error) {
            cerr << error.what() << "\n";
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
    } catch (const exception& error) {
        cerr << error.what() << "\n";
        exit(EXIT_FAILURE);
    } catch (...) {
        cerr << "An unknown error occurred.\n";
        exit(EXIT_FAILURE);
    }
}
