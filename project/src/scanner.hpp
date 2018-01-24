#pragma once

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "token.hpp"

namespace motts { namespace lox {
    class Scanner {
        public:
            explicit Scanner(const std::string& source);
            const std::vector<std::unique_ptr<Token>>& scan_tokens();

        private:
            std::string source_;

            // Nystrom tracks the substring of a token with two indexes, named `start` and `current`.
            // But in C++, it's considered better style to track positions with iterators.
            // After I changed the type of `start` and `current` to `string::iterator`,
            // it made sense to rename them based on iterator terminology,
            // so I renamed `start` and `current` to `token_begin` and `token_end` respectively.
            std::string::const_iterator token_begin_;
            std::string::const_iterator token_end_;

            int line_ {1};
            std::vector<std::unique_ptr<Token>> tokens_;

            void add_token(Token_type type);
            template<typename Literal_type>
                void add_token(Token_type type, const Literal_type& literal_value);

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
