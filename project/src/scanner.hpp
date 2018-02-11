#pragma once

#include <stdexcept>
#include <string>
#include <vector>

#include "token.hpp"

namespace motts { namespace lox {
    class Scanner {
        public:
            explicit Scanner(std::string&& source);
            const std::vector<Token>& scan_tokens();

        private:
            std::string source;

            // Nystrom tracks the substring of a token with two indexes, named `start` and
            // `current`. But in C++, it's considered better style to track positions with
            // iterators. After I changed the type of `start` and `current` to
            // `string::iterator`, it made sense to rename them based on iterator terminology,
            // so I renamed `start` and `current` to `token_begin` and `token_end`
            // respectively.
            std::string::const_iterator token_begin;
            std::string::const_iterator token_end;

            int line {1};
            std::vector<Token> tokens;

            void add_token(Token_type);
            void add_token(Token_type, Literal_multi_type&&);

            // I renamed `match` to `advance_if_match` to communicate the side-effect this function causes
            bool advance_if_match(char expected);

            void consume_string();
            void consume_number();
            void consume_identifier();
            void consume_token();
    };

    class Scanner_error : public std::runtime_error {
        public:
            Scanner_error(const std::string& what, int line);
    };
}}
