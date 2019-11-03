#include "scanner.hpp"

#include <cctype>
#include <stdexcept>
#include <utility>

#include <boost/algorithm/string.hpp>

using std::isalnum;
using std::isalpha;
using std::isdigit;
using std::logic_error;
using std::move;
using std::ostream;
using std::string;

using boost::is_any_of;
using boost::to_upper;
using boost::trim_right_if;

namespace motts { namespace lox {
    ostream& operator<<(ostream& os, Token_type token_type) {
        // IIFE so I can use return rather than assign-and-break
        string name {([&] () {
            switch (token_type) {
                #define X(name) \
                    case Token_type::name: \
                        return #name;
                MOTTS_LOX_TOKEN_TYPE_NAMES
                #undef X

                default:
                    throw logic_error{"Unexpected token type."};
            }
        })()};

        // Field names should print as uppercase without trailing underscores
        trim_right_if(name, is_any_of("_"));
        to_upper(name);

        os << name;

        return os;
    }

    bool operator==(const Token& lhs, const Token& rhs) {
        return (
            lhs.type == rhs.type &&
            lhs.begin == rhs.begin && lhs.end == rhs.end &&
            lhs.line == rhs.line
        );
    }

    Token_iterator::Token_iterator(const string& source) :
        token_begin_ {source.cbegin()},
        token_end_ {source.cbegin()},
        source_end_ {source.cend()},
        token_ {consume_token()}
    {}

    Token_iterator::Token_iterator() :
        token_ {Token_type::eof, {}, {}, {}}
    {}

    Token_iterator& Token_iterator::operator++() {
        token_ = consume_token();
        return *this;
    }

    Token_iterator Token_iterator::operator++(int) {
        Token_iterator copy {*this};
        operator++();

        return copy;
    }

    bool Token_iterator::operator==(const Token_iterator& rhs) const {
        return (token_.type == Token_type::eof && rhs.token_.type == Token_type::eof) || token_ == rhs.token_;
    }

    bool Token_iterator::operator!=(const Token_iterator& rhs) const {
        return !(*this == rhs);
    }

    const Token& Token_iterator::operator*() const & {
        return token_;
    }

    Token&& Token_iterator::operator*() && {
        return move(token_);
    }

    const Token* Token_iterator::operator->() const {
        return &token_;
    }

    Token Token_iterator::consume_token() {
        consume_whitespace();
        if (token_end_ == source_end_) {
            return Token{Token_type::eof, {}, {}, {}};
        }

        token_begin_ = token_end_;
        const auto c = *token_end_++;

        if (isalpha(c) || c == '_') return consume_identifier();
        if (isdigit(c)) return consume_number();

        switch (c) {
            case '(': return make_token(Token_type::left_paren);
            case ')': return make_token(Token_type::right_paren);
            case '{': return make_token(Token_type::left_brace);
            case '}': return make_token(Token_type::right_brace);
            case ';': return make_token(Token_type::semicolon);
            case ',': return make_token(Token_type::comma);
            case '.': return make_token(Token_type::dot);
            case '-': return make_token(Token_type::minus);
            case '+': return make_token(Token_type::plus);
            case '/': return make_token(Token_type::slash);
            case '*': return make_token(Token_type::star);
            case '!':
                return make_token(advance_if_match('=') ? Token_type::bang_equal : Token_type::bang);
            case '=':
                return make_token(advance_if_match('=') ? Token_type::equal_equal : Token_type::equal);
            case '<':
                return make_token(advance_if_match('=') ? Token_type::less_equal : Token_type::less);
            case '>':
                return make_token(advance_if_match('=') ? Token_type::greater_equal : Token_type::greater);
            case '"':
                return consume_string();
        }

        throw Scanner_error{"Unexpected character."};
    }

    void Token_iterator::consume_whitespace() {
        while (token_end_ != source_end_) {
            const auto c = *token_end_;

            switch (c) {
                case ' ':
                case '\r':
                case '\t':
                    ++token_end_;
                    continue;

                case '\n':
                    ++line_;
                    ++token_end_;
                    continue;

                case '/':
                    // If not two slashes, then not whitespace
                    if ((token_end_ + 1) == source_end_ || *(token_end_ + 1) != '/') {
                        return;
                    }

                    // Consume two slashes
                    token_end_ += 2;

                    while (token_end_ != source_end_ && *token_end_ != '\n') {
                        ++token_end_;
                    }

                    continue;

                default:
                    return;
            }
        }
    }

    Token Token_iterator::consume_identifier() {
        while (token_end_ != source_end_ && (isalnum(*token_end_) || *token_end_ == '_')) {
            ++token_end_;
        }

        return make_token(identifier_type());
    }

    Token Token_iterator::consume_number() {
        while (token_end_ != source_end_ && isdigit(*token_end_)) {
            ++token_end_;
        }

        // Look for fractional part
        if (
            token_end_ != source_end_ && *token_end_ == '.' &&
            (token_end_ + 1) != source_end_ && isdigit(*(token_end_ + 1))
        ) {
            // Consume the "." and digit
            token_end_ += 2;

            while (token_end_ != source_end_ && isdigit(*token_end_)) {
                ++token_end_;
            }
        }

        return make_token(Token_type::number);
    }

    Token Token_iterator::consume_string() {
        while (token_end_ != source_end_ && *token_end_ != '"') {
            if (*token_end_ == '\n') {
                ++line_;
            }
            ++token_end_;
        }

        if (token_end_ == source_end_) {
            throw Scanner_error{"Unterminated string."};
        }

        // The closing quote
        ++token_end_;

        return make_token(Token_type::string);
    }

    Token Token_iterator::make_token(Token_type type) {
        return Token{type, token_begin_, token_end_, line_};
    }

    bool Token_iterator::advance_if_match(char expected) {
        if (token_end_ == source_end_ || *token_end_ != expected) {
            return false;
        }

        ++token_end_;

        return true;
    }

    Token_type Token_iterator::identifier_type() {
        switch (*token_begin_) {
            case 'a': return keyword_or_identifier(token_begin_ + 1, "nd", Token_type::and_);
            case 'c': return keyword_or_identifier(token_begin_ + 1, "lass", Token_type::class_);
            case 'e': return keyword_or_identifier(token_begin_ + 1, "lse", Token_type::else_);
            case 'f':
                if ((token_begin_ + 1) != token_end_) {
                    switch (*(token_begin_ + 1)) {
                        case 'a': return keyword_or_identifier(token_begin_ + 2, "lse", Token_type::false_);
                        case 'o': return keyword_or_identifier(token_begin_ + 2, "r", Token_type::for_);
                        case 'u': return keyword_or_identifier(token_begin_ + 2, "n", Token_type::fun);
                    }
                }

                break;

            case 'i': return keyword_or_identifier(token_begin_ + 1, "f", Token_type::if_);
            case 'n': return keyword_or_identifier(token_begin_ + 1, "il", Token_type::nil);
            case 'o': return keyword_or_identifier(token_begin_ + 1, "r", Token_type::or_);
            case 'p': return keyword_or_identifier(token_begin_ + 1, "rint", Token_type::print);
            case 'r': return keyword_or_identifier(token_begin_ + 1, "eturn", Token_type::return_);
            case 's': return keyword_or_identifier(token_begin_ + 1, "uper", Token_type::super);
            case 't':
                if ((token_begin_ + 1) != token_end_) {
                    switch (*(token_begin_ + 1)) {
                        case 'h': return keyword_or_identifier(token_begin_ + 2, "is", Token_type::this_);
                        case 'r': return keyword_or_identifier(token_begin_ + 2, "ue", Token_type::true_);
                    }
                }

                break;

            case 'v': return keyword_or_identifier(token_begin_ + 1, "ar", Token_type::var);
            case 'w': return keyword_or_identifier(token_begin_ + 1, "hile", Token_type::while_);
        }

        return Token_type::identifier;
    }

    Token_type Token_iterator::keyword_or_identifier(
        string::const_iterator begin,
        const char* rest_keyword,
        Token_type type_if_match
    ) {
        if (string{begin, token_end_} == rest_keyword) {
            return type_if_match;
        }

        return Token_type::identifier;
    }
}}
