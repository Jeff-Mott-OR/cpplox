#include "scanner.hpp"

#include <cctype>
#include <stdexcept>
#include <string>

#include <boost/algorithm/string.hpp>

namespace motts::lox
{
    std::ostream& operator<<(std::ostream& os, Token_type token_type)
    {
        // Immediately invoked lambda lets me return from the switch rather than assign and break.
        const char* token_name = [&] {
            switch (token_type) {
                default: {
                    throw std::logic_error{"Unexpected token type."};
                }

#define X(name) \
    case Token_type::name: \
        return #name;
                    MOTTS_LOX_TOKEN_TYPE_NAMES
#undef X
            }
        }();

        // Names should print as uppercase without trailing underscores.
        std::string name_str{token_name};
        boost::trim_right_if(name_str, boost::is_any_of("_"));
        boost::to_upper(name_str);

        os << name_str;

        return os;
    }

    std::ostream& operator<<(std::ostream& os, const Token& token)
    {
        os << "Token { "
           << "type: " << token.type << ", "
           << "lexeme: " << token.lexeme << ", "
           << "line: " << token.line << " }";

        return os;
    }

    Token_iterator::Token_iterator(std::string_view source)
        : token_begin_{source.cbegin()},
          token_end_{source.cbegin()},
          source_end_{source.cend()},
          token_{scan_token()}
    {
    }

    Token_iterator::Token_iterator()
        : token_{Token_type::eof, "", 0}
    {
    }

    Token_iterator& Token_iterator::operator++()
    {
        token_ = scan_token();
        return *this;
    }

    Token_iterator Token_iterator::operator++(int)
    {
        const auto copy = *this;
        operator++();
        return copy;
    }

    const Token& Token_iterator::operator*() const
    {
        return token_;
    }

    const Token* Token_iterator::operator->() const
    {
        return &token_;
    }

    bool Token_iterator::operator==(const Token_iterator& rhs) const
    {
        // For now this token type comparison is good enough, since
        // all we ever need to check is whether we're at token type EOF.
        return token_.type == rhs.token_.type;
    }

    bool operator!=(const Token_iterator& lhs, const Token_iterator& rhs)
    {
        return ! (lhs == rhs);
    }

    // Advance the source iterators to capture the next token.
    Token Token_iterator::scan_token()
    {
        while (token_end_ != source_end_) {
            // Start the next token where the previous token left off.
            token_begin_ = token_end_;
            const auto next_char = *token_end_++;

            if (std::isalpha(next_char) || '_' == next_char) {
                return scan_identifier_token();
            }

            if (std::isdigit(next_char)) {
                return scan_number_token();
            }

            switch (next_char) {
                default:
                    throw std::runtime_error{"[Line " + std::to_string(line_) + "] Error: Unexpected character \"" + next_char + "\"."};

                case '"':
                    return scan_string_token();

                // Skip whitespace.
                case ' ':
                case '\r':
                case '\t':
                    continue;

                // Count lines then skip whitespace.
                case '\n':
                    ++line_;
                    continue;

                case '/':
                    // Two cosecutive slashes means line comment.
                    if (scan_if_match('/')) {
                        while (token_end_ != source_end_ && *token_end_ != '\n') {
                            ++token_end_;
                        }
                        continue;
                    }

                    // Else, single slash token.
                    return Token{Token_type::slash, {token_begin_, token_end_}, line_};

                case '(':
                    return Token{Token_type::left_paren, {token_begin_, token_end_}, line_};
                case ')':
                    return Token{Token_type::right_paren, {token_begin_, token_end_}, line_};
                case '{':
                    return Token{Token_type::left_brace, {token_begin_, token_end_}, line_};
                case '}':
                    return Token{Token_type::right_brace, {token_begin_, token_end_}, line_};
                case ',':
                    return Token{Token_type::comma, {token_begin_, token_end_}, line_};
                case '.':
                    return Token{Token_type::dot, {token_begin_, token_end_}, line_};
                case '-':
                    return Token{Token_type::minus, {token_begin_, token_end_}, line_};
                case '+':
                    return Token{Token_type::plus, {token_begin_, token_end_}, line_};
                case ';':
                    return Token{Token_type::semicolon, {token_begin_, token_end_}, line_};
                case '*':
                    return Token{Token_type::star, {token_begin_, token_end_}, line_};

                case '!':
                    return {scan_if_match('=') ? Token_type::bang_equal : Token_type::bang, {token_begin_, token_end_}, line_};
                case '=':
                    return {scan_if_match('=') ? Token_type::equal_equal : Token_type::equal, {token_begin_, token_end_}, line_};
                case '>':
                    return {scan_if_match('=') ? Token_type::greater_equal : Token_type::greater, {token_begin_, token_end_}, line_};
                case '<':
                    return {scan_if_match('=') ? Token_type::less_equal : Token_type::less, {token_begin_, token_end_}, line_};
            }
        }

        return Token{Token_type::eof, "EOF", line_};
    }

    Token Token_iterator::scan_identifier_token()
    {
        while (token_end_ != source_end_ && (std::isalnum(*token_end_) || '_' == *token_end_)) {
            ++token_end_;
        }

        // Check if this identifier is a keyword.
        const auto token_type = [&] {
            // string_view lets us use == with a c-string.
            const std::string_view token_lexeme{token_begin_, token_end_};

            if (token_lexeme == "and") return Token_type::and_;
            if (token_lexeme == "break") return Token_type::break_;
            if (token_lexeme == "class") return Token_type::class_;
            if (token_lexeme == "continue") return Token_type::continue_;
            if (token_lexeme == "else") return Token_type::else_;
            if (token_lexeme == "false") return Token_type::false_;
            if (token_lexeme == "for") return Token_type::for_;
            if (token_lexeme == "fun") return Token_type::fun;
            if (token_lexeme == "if") return Token_type::if_;
            if (token_lexeme == "nil") return Token_type::nil;
            if (token_lexeme == "or") return Token_type::or_;
            if (token_lexeme == "print") return Token_type::print;
            if (token_lexeme == "return") return Token_type::return_;
            if (token_lexeme == "super") return Token_type::super;
            if (token_lexeme == "this") return Token_type::this_;
            if (token_lexeme == "true") return Token_type::true_;
            if (token_lexeme == "var") return Token_type::var;
            if (token_lexeme == "while") return Token_type::while_;

            // Otherwise, just a generic non-keyword identifier.
            return Token_type::identifier;
        }();

        return Token{token_type, {token_begin_, token_end_}, line_};
    }

    Token Token_iterator::scan_number_token()
    {
        while (token_end_ != source_end_ && std::isdigit(*token_end_)) {
            ++token_end_;
        }

        // Fractional part.
        if (token_end_ != source_end_ && *token_end_ == '.' && (token_end_ + 1) != source_end_ && std::isdigit(*(token_end_ + 1))) {
            // Consume the "." and digit.
            token_end_ += 2;

            while (token_end_ != source_end_ && std::isdigit(*token_end_)) {
                ++token_end_;
            }
        }

        return Token{Token_type::number, {token_begin_, token_end_}, line_};
    }

    Token Token_iterator::scan_string_token()
    {
        const auto string_begin_line = line_;

        while (token_end_ != source_end_ && *token_end_ != '"') {
            if (*token_end_ == '\n') {
                ++line_;
            }
            ++token_end_;
        }

        if (token_end_ == source_end_) {
            throw std::runtime_error{"[Line " + std::to_string(line_) + "] Error: Unterminated string."};
        }

        // The closing quote.
        ++token_end_;

        return Token{Token_type::string, {token_begin_, token_end_}, string_begin_line};
    }

    bool Token_iterator::scan_if_match(char expected)
    {
        if (token_end_ != source_end_ && *token_end_ == expected) {
            ++token_end_;
            return true;
        }

        return false;
    }
}
