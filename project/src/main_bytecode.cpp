#include <cstdlib>

#include <algorithm>
#include <array>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <ostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <gsl/span>

#include "exception.hpp"
#include "parser.hpp"
#include "parser_bc.hpp"
#include "scanner.hpp"

using std::array;
using std::cerr;
using std::cin;
using std::cout;
using std::exception;
using std::exit;
using std::for_each;
using std::getline;
using std::ifstream;
using std::istreambuf_iterator;
using std::logic_error;
using std::ostream;
using std::setfill;
using std::setw;
using std::string;
using std::vector;

using boost::get;
using boost::is_any_of;
using boost::to_upper;
using boost::trim_right_if;
using gsl::span;

namespace lox = motts::lox;

ostream& operator<<(ostream& os, lox::Opcode opcode) {
    // IIFE so I can use return rather than assign-and-break
    string name {([&] () {
        switch (opcode) {
            #define X(name) \
                case lox::Opcode::name: \
                    return #name;
            MOTTS_LOX_OPCODE_NAMES
            #undef X

            default:
                throw logic_error{"Unexpected opcode."};
        }
    })()};

    // Field names should print as uppercase without trailing underscores
    trim_right_if(name, is_any_of("_"));
    to_upper(name);

    os << name;

    return os;
}

auto disassemble_instruction(const lox::Chunk& chunk, vector<int>::const_iterator ip) {
    const auto index = ip - chunk.code.cbegin();

    cout << setw(4) << setfill('0') << index << " ";
    if (index == 0 || chunk.lines.at(index) != chunk.lines.at(index - 1)) {
        cout << setw(4) << setfill(' ') << chunk.lines.at(index);
    } else {
        cout << setw(4) << setfill(' ') << '|';
    }

    const auto opcode = static_cast<lox::Opcode>(*(ip++));
    cout << " " << opcode;
    if (opcode == lox::Opcode::constant) {
        const auto constnat_index = *(ip++);
        cout << " " << constnat_index << " " << chunk.constants.at(constnat_index);
    }
    cout << "\n";

    return ip;
}

void disassemble_chunk(const lox::Chunk& chunk) {
    for (auto i = chunk.code.cbegin(); i != chunk.code.cend(); ) {
        i = disassemble_instruction(chunk, i);
    }
}

class VM {
    const lox::Chunk* chunk_ {};
    vector<int>::const_iterator ip_;
    array<double, 256> stack_;
    decltype(stack_)::iterator stack_top_ {stack_.begin()};

    void push(double value);
    double pop();

    public:
        lox::Chunk compile(const string& source);
        void interpret(const string& source);
};

lox::Chunk VM::compile(const string& source) {
    auto chunk = lox::parse_bc(lox::Token_iterator{source});

    chunk.code.push_back(static_cast<int>(lox::Opcode::return_));
    chunk.lines.push_back(-1);

    return chunk;
}

void VM::interpret(const string& source) {
    const auto chunk = compile(source);

    chunk_ = &chunk;
    ip_ = chunk.code.cbegin();

    while (true) {
        #ifndef NDEBUG
            for (auto i = stack_.begin(); i != stack_top_; ++i) {
                cout << "[ " << *i << " ]";
            }
            cout << "\n";

            disassemble_instruction(chunk, ip_);
        #endif

        const auto instruction = static_cast<lox::Opcode>(*(ip_++));

        switch (instruction) {
            case lox::Opcode::return_: {
                return;
            }

            case lox::Opcode::constant: {
                const auto constant = chunk.constants.at(*(ip_++));
                push(constant);
                break;
            }

            case lox::Opcode::negate: {
                push(-pop());
                break;
            }

            case lox::Opcode::add: push(pop() + pop()); break;
            case lox::Opcode::multiply: push(pop() * pop()); break;
            case lox::Opcode::subtract: {
                const auto b = pop();
                const auto a = pop();
                push(a - b);
                break;
            }
            case lox::Opcode::divide: {
                const auto b = pop();
                const auto a = pop();
                push(a / b);
                break;
            }

            case lox::Opcode::print: {
                cout << pop() << "\n";
                break;
            }
        }
    }
}

void VM::push(double value) {
    *stack_top_ = value;
    ++stack_top_;
}

double VM::pop() {
    --stack_top_;
    return *stack_top_;
}

auto run(const string& source, VM& vm) {
    vm.interpret(source);
}

auto run(const string& source) {
    VM vm;
    run(source, vm);
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
    VM vm;

    while (true) {
        cout << "> ";

        string source_line;
        getline(cin, source_line);

        // If the user makes a mistake, it shouldn't kill their entire session
        try {
            run(source_line, vm);
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
            cout << "Usage: cpploxbc [script]\n";
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
