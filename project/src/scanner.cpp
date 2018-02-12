#include "scanner.hpp"

#include <cctype>

#include <unordered_map>
#include <utility>

#include <boost/lexical_cast.hpp>

using std::isalnum;
using std::isalpha;
using std::isdigit;

using std::move;
using std::runtime_error;
using std::string;
using std::to_string;
using std::unordered_map;

using boost::lexical_cast;

using motts::lox::Token_type;

// Not exported (internal linkage)
namespace {
    // TODO: Under the microscope of profile and benchmarking tools, consider changing
    // this data structure to an array of tuples. The data is small enough that an
    // array may be faster.
    const unordered_map<string, Token_type> reserved_words {
        {"and", Token_type::and_},
        {"class", Token_type::class_},
        {"else", Token_type::else_},
        {"false", Token_type::false_},
        {"for", Token_type::for_},
        {"fun", Token_type::fun_},
        {"if", Token_type::if_},
        {"nil", Token_type::nil_},
        {"or", Token_type::or_},
        {"print", Token_type::print_},
        {"return", Token_type::return_},
        {"super", Token_type::super_},
        {"this", Token_type::this_},
        {"true", Token_type::true_},
        {"var", Token_type::var_},
        {"while", Token_type::while_},
    };
}

// Exported (external linkage)
namespace motts { namespace lox {
    Token_iterator::Token_iterator(const string& source_)
        : source {&source_},
          token_begin {source->cbegin()},
          token_end {source->cbegin()},
          token {consume_token()}
    {}

    Token_iterator::Token_iterator() = default;

    Token_iterator& Token_iterator::operator++() {
        if (token_begin != source->end()) {
            // We are at the beginning of the next lexeme
            token_begin = token_end;
            token = consume_token();
        } else {
            source = nullptr;
        }

        return *this;
    }

    Token_iterator Token_iterator::operator++(int) {
        Token_iterator copy {*this};
        operator++();

        return copy;
    }

    bool Token_iterator::operator==(const Token_iterator& rhs) const {
        return (
            (!source && !rhs.source) ||
            (source == rhs.source && token_begin == rhs.token_begin)
        );
    }

    bool Token_iterator::operator!=(const Token_iterator& rhs) const {
        return !(*this == rhs);
    }

    const Token& Token_iterator::operator*() const & {
        return token;
    }

    Token&& Token_iterator::operator*() && {
        return move(token);
    }

    const Token* Token_iterator::operator->() const {
        return &token;
    }

    Token Token_iterator::make_token(Token_type token_type) {
        return Token{token_type, string{token_begin, token_end}, {}, line};
    }

    Token Token_iterator::make_token(Token_type token_type, Literal_multi_type&& literal_value) {
        return Token{
            token_type,
            string{token_begin, token_end},
            move(literal_value),
            line
        };
    }

    bool Token_iterator::advance_if_match(char expected) {
        if (token_end != source->end() && *token_end == expected) {
            ++token_end;

            return true;
        }

        return false;
    }

    Token Token_iterator::consume_string() {
        while (token_end != source->end() && *token_end != '"') {
            if (*token_end == '\n') {
                ++line;
            }

            ++token_end;
        }

        // Check unterminated string
        if (token_end == source->end()) {
            throw Scanner_error{"Unterminated string.", line};
        }

        // The closing "
        ++token_end;

        // Trim the surrounding quotes for literal value
        return make_token(Token_type::string, Literal_multi_type{string{token_begin + 1, token_end - 1}});
    }

    Token Token_iterator::consume_number() {
        while (token_end != source->end() && isdigit(*token_end)) {
            ++token_end;
        }

        // Look for a fractional part
        if (
            token_end != source->end() && *token_end ==  '.' &&
            (token_end + 1) != source->end() && isdigit(*(token_end + 1))
        ) {
            // Consume the "."
            ++token_end;

            while (token_end != source->end() && isdigit(*token_end)) {
                ++token_end;
            }
        }

        return make_token(Token_type::number, Literal_multi_type{lexical_cast<double>(string{token_begin, token_end})});
    }

    Token Token_iterator::consume_identifier() {
        while (token_end != source->end() && (isalnum(*token_end) || *token_end == '_')) {
            ++token_end;
        }

        const auto found = reserved_words.find(string{token_begin, token_end});
        return make_token(
            found != reserved_words.end() ?
                found->second :
                Token_type::identifier
        );
    }

    Token Token_iterator::consume_token() {
        // Loop because we might skip some tokens
        for (; token_begin != source->end(); operator++()) {
            auto c = *token_end;
            ++token_end;

            switch (c) {
                // Single char tokens
                case '(': return make_token(Token_type::left_paren);
                case ')': return make_token(Token_type::right_paren);
                case '{': return make_token(Token_type::left_brace);
                case '}': return make_token(Token_type::right_brace);
                case ',': return make_token(Token_type::comma);
                case '.': return make_token(Token_type::dot);
                case '-': return make_token(Token_type::minus);
                case '+': return make_token(Token_type::plus);
                case ';': return make_token(Token_type::semicolon);
                case '*': return make_token(Token_type::star);

                // One or two char tokens
                case '/':
                    if (advance_if_match('/')) {
                        // A comment goes until the end of the line
                        while (token_end != source->end() && *token_end != '\n') {
                            ++token_end;
                        }

                        continue;
                    } else {
                        return make_token(Token_type::slash);
                    }

                case '!':
                    return make_token(
                        advance_if_match('=') ?
                            Token_type::bang_equal :
                            Token_type::bang
                    );

                case '=':
                    return make_token(
                        advance_if_match('=') ?
                            Token_type::equal_equal :
                            Token_type::equal
                    );

                case '>':
                    return make_token(
                        advance_if_match('=') ?
                            Token_type::greater_equal :
                            Token_type::greater
                    );

                case '<':
                    return make_token(
                        advance_if_match('=') ?
                            Token_type::less_equal :
                            Token_type::less
                    );

                // Whitespace
                case '\n':
                    ++line;

                    continue;

                case ' ':
                case '\r':
                case '\t':
                    continue;

                // Literals and Keywords
                case '"': return consume_string();
                default:
                    if (isdigit(c)) {
                        return consume_number();
                    } else if (isalpha(c) || c == '_') {
                        return consume_identifier();
                    } else {
                        throw Scanner_error{"Unexpected character.", line};
                    }
            }
        }

        // The final token is always EOF
        return Token{Token_type::eof, "", {}, line};
    }

    Scanner_error::Scanner_error(const string& what, int line)
        : runtime_error {"[Line " + to_string(line) + "] Error: " + what}
    {}
}}
