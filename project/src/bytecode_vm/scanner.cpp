#include "scanner.hpp"

#include <cctype>
#include <stdexcept>
#include <string>

namespace motts { namespace lox {
    Token_iterator::Token_iterator(const gsl::cstring_span<>& source) :
        token_begin_ {source.data()},
        token_end_ {source.data()},
        source_end_ {source.data() + source.size()},
        token_ {consume_token()}
    {
    }

    const Token& Token_iterator::operator*() const {
        return token_;
    }

    const Token* Token_iterator::operator->() const {
        return &token_;
    }

    Token_iterator& Token_iterator::operator++() {
        token_ = consume_token();
        return *this;
    }

    Token_iterator Token_iterator::operator++(int) {
        const auto copy = *this;
        operator++();
        return copy;
    }

    Token Token_iterator::consume_token() {
        while (token_end_ != source_end_) {
            token_begin_ = token_end_;
            const auto char_ = *token_end_++;

            if (std::isalpha(char_) || char_ == '_') {
                return consume_identifier();
            }

            if (std::isdigit(char_)) {
                return consume_number();
            }

            switch (char_) {
                default:
                    throw std::runtime_error{"[Line " + std::to_string(line_) + "] Error: Unexpected character."};

                // Skip whitespace
                case ' ':
                case '\r':
                case '\t':
                    continue;

                // Count lines then skip whitespace
                case '\n':
                    ++line_;
                    continue;

                // Slash or line comment
                case '/':
                    // Two consecutive slashes means line comment
                    if (consume_if_match('/')) {
                        while (token_end_ != source_end_ && *token_end_ != '\n') {
                            ++token_end_;
                        }
                        continue;
                    }

                    // Otherwise, single slash token
                    return {Token_type::slash, {token_begin_, token_end_}, line_};

                case '"':
                    return consume_string();

                case '!': return {consume_if_match('=') ? Token_type::bang_equal : Token_type::bang, {token_begin_, token_end_}, line_};
                case '=': return {consume_if_match('=') ? Token_type::equal_equal : Token_type::equal, {token_begin_, token_end_}, line_};
                case '<': return {consume_if_match('=') ? Token_type::less_equal : Token_type::less, {token_begin_, token_end_}, line_};
                case '>': return {consume_if_match('=') ? Token_type::greater_equal : Token_type::greater, {token_begin_, token_end_}, line_};

                case '(': return {Token_type::left_paren, {token_begin_, token_end_}, line_};
                case ')': return {Token_type::right_paren, {token_begin_, token_end_}, line_};
                case '{': return {Token_type::left_brace, {token_begin_, token_end_}, line_};
                case '}': return {Token_type::right_brace, {token_begin_, token_end_}, line_};
                case ';': return {Token_type::semicolon, {token_begin_, token_end_}, line_};
                case ',': return {Token_type::comma, {token_begin_, token_end_}, line_};
                case '.': return {Token_type::dot, {token_begin_, token_end_}, line_};
                case '-': return {Token_type::minus, {token_begin_, token_end_}, line_};
                case '+': return {Token_type::plus, {token_begin_, token_end_}, line_};
                case '*': return {Token_type::star, {token_begin_, token_end_}, line_};
            }
        }

        return Token{Token_type::eof, nullptr, line_};
    }

    Token Token_iterator::consume_identifier() {
        while (
            token_end_ != source_end_ &&
            (std::isalnum(*token_end_) || *token_end_ == '_')
        ) {
            ++token_end_;
        }

        // For ease of writing comparisons
        gsl::cstring_span<> token_str {token_begin_, token_end_};

        const auto token_type = ([&] () {
            if (token_str == "and") return Token_type::and_;
            if (token_str == "class") return Token_type::class_;
            if (token_str == "else") return Token_type::else_;
            if (token_str == "false") return Token_type::false_;
            if (token_str == "for") return Token_type::for_;
            if (token_str == "fun") return Token_type::fun_;
            if (token_str == "if") return Token_type::if_;
            if (token_str == "nil") return Token_type::nil;
            if (token_str == "or") return Token_type::or_;
            if (token_str == "print") return Token_type::print;
            if (token_str == "return") return Token_type::return_;
            if (token_str == "super") return Token_type::super_;
            if (token_str == "this") return Token_type::this_;
            if (token_str == "true") return Token_type::true_;
            if (token_str == "var") return Token_type::var_;
            if (token_str == "while") return Token_type::while_;
            return Token_type::identifier;
        })();

        return {token_type, {token_begin_, token_end_}, line_};
    }

    Token Token_iterator::consume_number() {
        while (token_end_ != source_end_ && std::isdigit(*token_end_)) {
            ++token_end_;
        }

        // Look for a fractional part
        if (
            token_end_ != source_end_ && *token_end_ == '.' &&
            (token_end_ + 1) != source_end_ && std::isdigit(*(token_end_ + 1))
        ) {
            // Consume the "." and digit
            token_end_ += 2;

            while (token_end_ != source_end_ && std::isdigit(*token_end_)) {
                ++token_end_;
            }
        }

        return {Token_type::number, {token_begin_, token_end_}, line_};
    }

    Token Token_iterator::consume_string() {
        while (token_end_ != source_end_ && *token_end_ != '"') {
            if (*token_end_ == '\n') {
                ++line_;
            }
            ++token_end_;
        }

        if (token_end_ == source_end_) {
            throw std::runtime_error{"[Line " + std::to_string(line_) + "] Error: Unterminated string."};
        }

        // The closing quote
        ++token_end_;

        return {Token_type::string, {token_begin_, token_end_}, line_};
    }

    bool Token_iterator::consume_if_match(char expected) {
        if (token_end_ != source_end_ && *token_end_ == expected) {
            ++token_end_;
            return true;
        }

        return false;
    }
}}
