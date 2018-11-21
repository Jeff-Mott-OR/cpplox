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

using boost::is_any_of;
using boost::to_upper;
using boost::trim_right_if;
using gsl::span;

namespace lox = motts::lox;

// X-macro technique
#define MOTTS_LOX_OPCODE_NAMES \
    X(return_) X(constant) X(negate) \
    X(add) X(subtract) X(multiply) X(divide)

enum class Opcode {
    #define X(name) name,
    MOTTS_LOX_OPCODE_NAMES
    #undef X
};

ostream& operator<<(ostream& os, Opcode opcode) {
    // IIFE so I can use return rather than assign-and-break
    string name {([&] () {
        switch (opcode) {
            #define X(name) \
                case Opcode::name: \
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

struct Chunk {
    vector<int> code;
    vector<double> constants;
    vector<int> lines;
};

auto disassemble_instruction(const Chunk& chunk, vector<int>::const_iterator ip) {
    const auto index = ip - chunk.code.cbegin();

    cout << setw(4) << setfill('0') << index << " ";
    if (index == 0 || chunk.lines.at(index) != chunk.lines.at(index - 1)) {
        cout << setw(4) << setfill(' ') << chunk.lines.at(index);
    } else {
        cout << setw(4) << setfill(' ') << '|';
    }

    const auto opcode = static_cast<Opcode>(*(ip++));
    cout << " " << opcode;
    switch (opcode) {
        case Opcode::constant: {
            const auto constnat_index = *(ip++);
            cout << " " << constnat_index << " " << chunk.constants.at(constnat_index);
            break;
        }
    }
    cout << "\n";

    return ip;
}

void disassemble_chunk(const Chunk& chunk) {
    for (auto i = chunk.code.cbegin(); i != chunk.code.cend(); ) {
        i = disassemble_instruction(chunk, i);
    }
}

class VM {
    const Chunk* chunk_ {};
    vector<int>::const_iterator ip_;
    array<double, 256> stack_;
    decltype(stack_)::iterator stack_top_ {stack_.begin()};

    void push(double value);
    double pop();

    public:
        void compile(const string& source);
        void interpret(const Chunk&);
};

void VM::compile(const string& source) {
    auto prev_line = -1;
    // Nystrom started writing a new scanner, partly because he was writing in a new language, and partly because he
    // wanted a new feature, scanning on demand. In my case, however, I didn't need to switch languages, and my scanner
    // already scans on demand. That my scanner already scans on demand was a happy accident from structuring the
    // scanner as an iterator. That means I don't need to re-implement the scanner at all. I can reuse the one I already
    // have.
    for_each(lox::Token_iterator{source}, lox::Token_iterator{}, [&] (const auto& token) {
        if (token.line != prev_line) {
            cout << setw(4) << token.line << " ";
            prev_line = token.line;
        } else {
            cout << "   | ";
        }
        cout << token.type << " '" << token.lexeme << "'\n";
    });
}

void VM::interpret(const Chunk& chunk) {
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

        const auto instruction = static_cast<Opcode>(*(ip_++));

        switch (instruction) {
            case Opcode::return_: {
                cout << pop() << "\n";
                return;
            }

            case Opcode::constant: {
                const auto constant = chunk.constants.at(*(ip_++));
                push(constant);
                break;
            }

            case Opcode::negate: {
                push(-pop());
                break;
            }

            case Opcode::add: push(pop() + pop()); break;
            case Opcode::multiply: push(pop() * pop()); break;
            case Opcode::subtract: {
                const auto b = pop();
                const auto a = pop();
                push(a - b);
                break;
            }
            case Opcode::divide: {
                const auto b = pop();
                const auto a = pop();
                push(a / b);
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
    vm.compile(source);
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
