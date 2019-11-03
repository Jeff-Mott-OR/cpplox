#include "value.hpp"
#include <iostream>

using std::cout;

namespace motts { namespace lox {
    void print_value(Value value) {
        cout << value;
    }
}}
