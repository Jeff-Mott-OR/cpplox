#include <iomanip>
#include <iostream>
#include <ostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/algorithm/string.hpp>

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
    X(return_) X(constant)

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

void disassemble_chunk(const Chunk& chunk) {
    for (auto i = chunk.code.cbegin(); i != chunk.code.cend(); ++i) {
        const auto index = i - chunk.code.cbegin();

        cout << setw(4) << setfill('0') << index << " ";
        if (index == 0 || chunk.lines.at(index) != chunk.lines.at(index - 1)) {
            cout << setw(4) << setfill(' ') << chunk.lines.at(index);
        } else {
            cout << setw(4) << setfill(' ') << '|';
        }

        const auto opcode = static_cast<Opcode>(*i);
        cout << " " << opcode;

        switch (opcode) {
            case Opcode::return_:
                break;

            case Opcode::constant:
                const auto constnat_index = *(++i);
                cout << " " << constnat_index << " " << chunk.constants.at(constnat_index);
                break;
        }

        cout << "\n";
    }
}

int main(/*int argc, const char* argv[]*/) {
    Chunk chunk;

    chunk.code.push_back(static_cast<int>(Opcode::return_));
    chunk.lines.push_back(123);

    chunk.constants.push_back(1.2);
    chunk.code.push_back(static_cast<int>(Opcode::constant));
    chunk.code.push_back(chunk.constants.cend() - chunk.constants.cbegin() - 1);
    chunk.lines.push_back(123);
    chunk.lines.push_back(123);

    disassemble_chunk(chunk);
}
