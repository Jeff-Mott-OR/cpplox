#include "scanner.hpp"

#include <cctype>
#include <stdexcept>
#include <string>

#include <boost/algorithm/string.hpp>

namespace motts { namespace lox {
    std::ostream& operator<<(std::ostream& os, const Token_type& token_type) {
        std::string name {([&] () {
            switch (token_type) {
                default:
                    throw std::logic_error{"Unexpected token type."};

                #define X(name) \
                    case Token_type::name: \
                        return #name;
                MOTTS_LOX_TOKEN_TYPE_NAMES
                #undef X
            }
        })()};

        // Names should print as uppercase without trailing underscores
        boost::trim_right_if(name, boost::is_any_of("_"));
        boost::to_upper(name);

        os << name;

        return os;
    }

    std::ostream& operator<<(std::ostream& os, const Token& token) {
        os << "Token { "
           << "type: " << token.type << ", "
           << "lexeme: " << token.lexeme << ", "
           << "line: " << token.line << " }";

       return os;
    }

    Token_iterator::Token_iterator(std::string_view source)
        : token_begin_ {source.cbegin()},
          token_end_ {source.cbegin()},
          source_end_ {source.cend()},
          token_ {scan_token()}
    {
    }

    Token_iterator::Token_iterator()
        : token_ {Token_type::eof, "", 0}
    {}

    Token_iterator& Token_iterator::operator++() {
        token_ = scan_token();
        return *this;
    }

    bool Token_iterator::operator==(const Token_iterator& rhs) const {
        return token_.type == rhs.token_.type;
    }

    bool operator!=(const Token_iterator& lhs, const Token_iterator& rhs) {
        return !(lhs == rhs);
    }

    const Token& Token_iterator::operator*() const {
        return token_;
    }

    const Token* Token_iterator::operator->() const {
        return &token_;
    }

    Token Token_iterator::scan_token() {
        while (token_end_ != source_end_) {
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
                    throw std::runtime_error{
                        "[Line " + std::to_string(line_) + "] Error: Unexpected character \"" + next_char + "\"."
                    };

                // Skip whitespace
                case ' ':
                    continue;

                // Count lines then skip whitespace
                case '\n':
                    ++line_;
                    continue;

                // Slash or line comment
                case '/':
                    // Two cosecutive slashes means line comment
                    if (scan_if_match('/')) {
                        while (token_end_ != source_end_ && *token_end_ != '\n') {
                            ++token_end_;
                        }

                        continue;
                    }

                    // Else, single slash token
                    return Token{Token_type::slash, {token_begin_, token_end_}, line_};

                case '(': return Token{Token_type::left_paren, {token_begin_, token_end_}, line_};
                case ')': return Token{Token_type::right_paren, {token_begin_, token_end_}, line_};
                case '{': return Token{Token_type::left_brace, {token_begin_, token_end_}, line_};
                case '}': return Token{Token_type::right_brace, {token_begin_, token_end_}, line_};
                case ',': return Token{Token_type::comma, {token_begin_, token_end_}, line_};
                case '.': return Token{Token_type::dot, {token_begin_, token_end_}, line_};
                case '-': return Token{Token_type::minus, {token_begin_, token_end_}, line_};
                case '+': return Token{Token_type::plus, {token_begin_, token_end_}, line_};
                case ';': return Token{Token_type::semicolon, {token_begin_, token_end_}, line_};
                case '*': return Token{Token_type::star, {token_begin_, token_end_}, line_};
            }
        }

        return Token{Token_type::eof, "", 0};
    }

    Token Token_iterator::scan_identifier_token() {
        while (token_end_ != source_end_ && (std::isalnum(*token_end_) || '_' == *token_end_)) {
            ++token_end_;
        }

        std::string_view token_lexeme {token_begin_, token_end_};

        const auto token_type = ([&] () {
            if ("and" == token_lexeme) return Token_type::and_;
            if ("or" == token_lexeme) return Token_type::or_;
            return Token_type::identifier;
        })();

        return Token{token_type, token_lexeme, line_};
    }

    Token Token_iterator::scan_number_token() {
        while (token_end_ != source_end_ && std::isdigit(*token_end_)) {
            ++token_end_;
        }

        // Fractional part
        if (
            token_end_ != source_end_ && '.' == *token_end_ &&
            (token_end_ + 1) != source_end_ && std::isdigit(*(token_end_ + 1))
        ) {
            // Consume the "." and digit
            token_end_ += 2;

            while (token_end_ != source_end_ && std::isdigit(*token_end_)) {
                ++token_end_;
            }
        }

        return Token{Token_type::number, {token_begin_, token_end_}, line_};
    }

    bool Token_iterator::scan_if_match(char expected) {
        if (token_end_ != source_end_ && *token_end_ == expected) {
            ++token_end_;
            return true;
        }

        return false;
    }
}}
