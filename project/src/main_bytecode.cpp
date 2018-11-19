#include <array>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/algorithm/string.hpp>

using std::array;
using std::cout;
using std::string;
using std::logic_error;
using std::ostream;
using std::setfill;
using std::setw;
using std::vector;

using boost::is_any_of;
using boost::to_upper;
using boost::trim_right_if;

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
    array<double, 256>::iterator stack_top_ {stack_.begin()};

    void push(double value);
    double pop();

    public:
        void interpret(const Chunk&);
};

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

int main(/*int argc, const char* argv[]*/) {
    Chunk chunk;

    chunk.constants.push_back(1.2);
    chunk.code.push_back(static_cast<int>(Opcode::constant));
    chunk.code.push_back(chunk.constants.cend() - chunk.constants.cbegin() - 1);
    chunk.lines.push_back(123);
    chunk.lines.push_back(123);

    chunk.constants.push_back(3.4);
    chunk.code.push_back(static_cast<int>(Opcode::constant));
    chunk.code.push_back(chunk.constants.cend() - chunk.constants.cbegin() - 1);
    chunk.lines.push_back(123);
    chunk.lines.push_back(123);

    chunk.code.push_back(static_cast<int>(Opcode::add));
    chunk.lines.push_back(123);

    chunk.constants.push_back(5.6);
    chunk.code.push_back(static_cast<int>(Opcode::constant));
    chunk.code.push_back(chunk.constants.cend() - chunk.constants.cbegin() - 1);
    chunk.lines.push_back(123);
    chunk.lines.push_back(123);

    chunk.code.push_back(static_cast<int>(Opcode::divide));
    chunk.lines.push_back(123);

    chunk.code.push_back(static_cast<int>(Opcode::return_));
    chunk.lines.push_back(123);

    disassemble_chunk(chunk);

    VM vm;
    vm.interpret(chunk);
}
