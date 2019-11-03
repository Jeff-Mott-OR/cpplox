#include "compiler.hpp"

#include <algorithm>
#include <iomanip>
#include <iostream>

#include "scanner.hpp"

using std::cout;
using std::for_each;
using std::setw;
using std::string;

namespace motts { namespace lox {
    void compile(const string& source) {
        Scanner scanner {source};

        auto prev_line = 0;
        while (true) {
            const auto token = scanner.scan_token();

            if (token.line != prev_line) {
                cout << setw(4) << token.line << " ";
                prev_line = token.line;
            } else {
                cout << "   | ";
            }

            cout << token.type << " '";
            for_each(token.begin, token.end, [] (auto c) {
                cout << c;
            });
            cout << "'\n";

            if (token.type == Token_type::eof) {
                break;
            }
        }
    }
}}
