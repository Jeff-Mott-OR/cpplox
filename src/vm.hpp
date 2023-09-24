#pragma once

#include <iostream>
#include <ostream>

#include "compiler.hpp"

namespace motts { namespace lox {
    void run(const Chunk&, std::ostream& = std::cout, bool debug = false);
}}
