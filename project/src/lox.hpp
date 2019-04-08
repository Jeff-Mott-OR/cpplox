#pragma once

#pragma warning(push, 0)
    #include <deferred_heap.h>
#pragma warning(pop)

#include "interpreter.hpp"
#include "parser.hpp"
#include "resolver.hpp"

namespace motts { namespace lox {
    struct Lox {
        gcpp::deferred_heap deferred_heap;

        std::function<std::vector<gcpp::deferred_ptr<const Stmt>>(Token_iterator&&)> parse {[&] (Token_iterator&& token_iter) {
            return ::motts::lox::parse(deferred_heap, move(token_iter));
        }};

        Interpreter interpreter {deferred_heap};

        Resolver resolver {interpreter};

        Lox() {
            deferred_heap.set_collect_before_expand(true);
        }
    };
}}
