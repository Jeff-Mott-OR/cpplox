#include <cstdlib>

#include <exception>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

#include <gsl/span>

#include "exception.hpp"
#include "lox.hpp"
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

using gsl::span;

namespace loxns = motts::lox;

// Not exported (internal linkage)
namespace {
    auto run(const string& source, loxns::Lox& lox) {
        const auto statements = lox.parse(loxns::Token_iterator{source});

        string resolver_errors;
        for (const auto& statement : statements) {
            try {
                statement->accept(statement, lox.resolver);
            } catch (const loxns::Resolver_error& error) {
                resolver_errors += error.what();
                resolver_errors += "\n";
            }
        }
        if (!resolver_errors.empty()) {
            throw loxns::Resolver_error{resolver_errors};
        }

        for (const auto& statement : statements) {
            statement->accept(statement, lox.interpreter);
        }
    }

    auto run(const string& source) {
        loxns::Lox lox;
        run(source, lox);
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
        loxns::Lox lox;

        while (true) {
            cout << "> ";

            string source_line;
            getline(cin, source_line);

            // If the user makes a mistake, it shouldn't kill their entire session
            try {
                run(source_line, lox);
            } catch (const loxns::Runtime_error& error) {
                cerr << error.what() << "\n";
            }
        }
    }
}

int main(int argc, const char* argv[]) {
    try {
        // STL-like container interface to argv
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
