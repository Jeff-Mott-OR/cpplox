#pragma once

#include <ostream>
#include <string>

namespace motts { namespace lox {
    void run_file(const std::string& input_file, std::ostream& os, bool debug);

    void run_prompt(bool debug);
}}
