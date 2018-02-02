#include <cstdlib>

#include <exception>
#include <fstream>
#include <ios>
#include <iostream>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

#include <gsl/span>

#include "ast_printer.hpp"
#include "parser.hpp"
#include "token.hpp"
#include "scanner.hpp"

using std::cin;
using std::cout;
using std::exception;
using std::exit;
using std::getline;
using std::ifstream;
using std::istream_iterator;
using std::move;
using std::noskipws;
using std::string;
using std::vector;

using gsl::span;

using motts::lox::Ast_printer;
using motts::lox::Parser;
using motts::lox::Scanner;
using motts::lox::Scanner_error;
using motts::lox::Token;

auto run(string&& source) {
    Scanner scanner {move(source)};
    Parser parser {vector<Token>{scanner.scan_tokens()}};
    const auto expression = parser.parse();

    Ast_printer ast_printer;
    expression->accept(ast_printer);
    cout << ast_printer.result() << "\n";
}

auto run_file(const string& path) {
    // IIFE to limit scope of ifstream
    auto source = ([&] () {
        ifstream in {path};
        in.exceptions(ifstream::failbit | ifstream::badbit);
        in >> noskipws;

        // Reaching eof will set the failbit (for some damn reason), and therefore also trigger an exception,
        // so temporarily disable exceptions, then check if we "failed" by eof before re-enabling exceptions.
        in.exceptions(ifstream::goodbit);
        const string source {istream_iterator<char>{in}, istream_iterator<char>{}};
        if (in.fail() && in.eof()) {
            in.clear();
        }
        in.exceptions(ifstream::failbit | ifstream::badbit);

        return source;
    })();

    run(move(source));
}

auto run_prompt() {
    while (true) {
        cout << "> ";

        string source_line;
        getline(cin, source_line);

        // If the user makes a mistake, it shouldn't kill their entire session
        try {
            run(move(source_line));
        } catch (const Scanner_error& e) {
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
