#include <cctype>

#include <stdio.h>
#include <string.h>

#include <gsl/string_span>

#include "common.hpp"
#include "scanner.hpp"

namespace {
    auto consume_if_match(Scanner& scanner, char expected) {
        if (scanner.token_end != scanner.source_end && *scanner.token_end == expected) {
            ++scanner.token_end;
            return true;
        }

        return false;
    }

    auto consume_identifier(Scanner& scanner) {
        while (
            scanner.token_end != scanner.source_end &&
            (std::isalnum(*scanner.token_end) || *scanner.token_end == '_')
        ) {
            ++scanner.token_end;
        }

        gsl::cstring_span<> token_str {scanner.token_begin, scanner.token_end};
        const auto token_type = ([&] () {
            if (token_str == "and") return Token_type::and_;
            if (token_str == "class") return Token_type::class_;
            if (token_str == "else") return Token_type::else_;
            if (token_str == "false") return Token_type::false_;
            if (token_str == "for") return Token_type::for_;
            if (token_str == "fun") return Token_type::fun_;
            if (token_str == "if") return Token_type::if_;
            if (token_str == "nil") return Token_type::nil_;
            if (token_str == "or") return Token_type::or_;
            if (token_str == "print") return Token_type::print_;
            if (token_str == "return") return Token_type::return_;
            if (token_str == "super") return Token_type::super_;
            if (token_str == "this") return Token_type::this_;
            if (token_str == "true") return Token_type::true_;
            if (token_str == "var") return Token_type::var_;
            if (token_str == "while") return Token_type::while_;
            return Token_type::identifier_;
        })();

        return Token{token_type, scanner.token_begin, scanner.token_end, scanner.line};
    }

    auto consume_number(Scanner& scanner) {
        while (scanner.token_end != scanner.source_end && std::isdigit(*scanner.token_end)) {
            ++scanner.token_end;
        }

        // Look for a fractional part.
        if (
            scanner.token_end != scanner.source_end && *scanner.token_end == '.' &&
            (scanner.token_end + 1) != scanner.source_end && std::isdigit(*(scanner.token_end + 1))
        ) {
            // Consume the "." and digit
            scanner.token_end += 2;

            while (scanner.token_end != scanner.source_end && std::isdigit(*scanner.token_end)) {
                ++scanner.token_end;
            }
        }

        return Token{Token_type::number_, scanner.token_begin, scanner.token_end, scanner.line};
    }

    auto consume_string(Scanner& scanner) {
        while (scanner.token_end != scanner.source_end && *scanner.token_end != '"') {
            if (*scanner.token_end == '\n') {
                ++scanner.line;
            }
            ++scanner.token_end;
        }

        if (scanner.token_end != scanner.source_end) {
            // The closing quote
            ++scanner.token_end;

            return Token{Token_type::string_, scanner.token_begin, scanner.token_end, scanner.line};
        }

        // TODO DRY
        throw std::runtime_error{
            "[Line " +
            std::to_string(scanner.line) +
            "] Error: Unterminated string."
        };
    }
}

Token scanToken(Scanner& scanner) {
    while (true) {
        if (scanner.token_end == scanner.source_end) {
            return Token{Token_type::eof_, scanner.source_end, scanner.source_end, scanner.line};
        }

        scanner.token_begin = scanner.token_end;

        const auto c = *scanner.token_end;
        ++scanner.token_end;

        if (std::isalpha(c) || c == '_') {
            return consume_identifier(scanner);
        }

        if (std::isdigit(c)) {
            return consume_number(scanner);
        }

        switch (c) {
            case '"': return consume_string(scanner);

            // Skip whitespace
            case ' ':
            case '\r':
            case '\t':
                continue;

            // Count lines then skip whitespace
            case '\n':
                ++scanner.line;
                continue;

            // Slash or line comment
            case '/':
                // Skip comments
                if (consume_if_match(scanner, '/')) {
                    while (scanner.token_end != scanner.source_end && *scanner.token_end != '\n') {
                        ++scanner.token_end;
                    }
                    continue;
                }

                return Token{Token_type::slash_, scanner.token_begin, scanner.token_end, scanner.line};

            case '(': return Token{Token_type::left_paren_, scanner.token_begin, scanner.token_end, scanner.line};
            case ')': return Token{Token_type::right_paren_, scanner.token_begin, scanner.token_end, scanner.line};
            case '{': return Token{Token_type::left_brace_, scanner.token_begin, scanner.token_end, scanner.line};
            case '}': return Token{Token_type::right_brace_, scanner.token_begin, scanner.token_end, scanner.line};
            case ';': return Token{Token_type::semicolon_, scanner.token_begin, scanner.token_end, scanner.line};
            case ',': return Token{Token_type::comma_, scanner.token_begin, scanner.token_end, scanner.line};
            case '.': return Token{Token_type::dot_, scanner.token_begin, scanner.token_end, scanner.line};
            case '-': return Token{Token_type::minus_, scanner.token_begin, scanner.token_end, scanner.line};
            case '+': return Token{Token_type::plus_, scanner.token_begin, scanner.token_end, scanner.line};
            case '*': return Token{Token_type::star_, scanner.token_begin, scanner.token_end, scanner.line};
            case '!': return Token{consume_if_match(scanner, '=') ? Token_type::bang_equal_ : Token_type::bang_, scanner.token_begin, scanner.token_end, scanner.line};
            case '=': return Token{consume_if_match(scanner, '=') ? Token_type::equal_equal_ : Token_type::equal_, scanner.token_begin, scanner.token_end, scanner.line};
            case '<': return Token{consume_if_match(scanner, '=') ? Token_type::less_equal_ : Token_type::less_, scanner.token_begin, scanner.token_end, scanner.line};
            case '>': return Token{consume_if_match(scanner, '=') ? Token_type::greater_equal_ : Token_type::greater_, scanner.token_begin, scanner.token_end, scanner.line};
        }

        throw std::runtime_error{
            "[Line " +
            std::to_string(scanner.line) +
            "] Error: Unexpected character."
        };
    }
}
