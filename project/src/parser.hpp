#pragma once

#include <string>
#include <vector>

#include "exception.hpp"
#include "scanner.hpp"
#include "statement.hpp"
#include "token.hpp"

namespace motts { namespace lox {
    /*
    Nystrom's Java code uses a `Parser` class with a single public method `parse`. That may be because Java has no free
    functions; everything must be in a class. In earlier commits, I mirrored that implementation, but in C++, free
    functions are not only possible but preferred, so I replaced the `Parser` class with a single `parse` free function.

    Also, now that the scanner has been refactored into a token iterator, the parser can accept and operate on a token
    iterator directly rather than on a vector of tokens. This way I was able to eliminate the intermediate data
    structure altogether.
    */
    std::vector<const Stmt*> parse(Token_iterator&&);

    struct Parser_error : Runtime_error {
        explicit Parser_error(const std::string& what, const Token&);
    };
}}
