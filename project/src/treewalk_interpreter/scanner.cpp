#include "scanner.hpp"

#include <cctype>

#include <algorithm>
#include <array>
#include <iterator>
#include <utility>

#include <boost/lexical_cast.hpp>

using std::isalnum;
using std::isalpha;
using std::isdigit;

using std::array;
using std::back_inserter;
using std::copy_if;
using std::find_if;
using std::move;
using std::pair;
using std::string;
using std::to_string;

using boost::lexical_cast;

// Allow the internal linkage section to access names
using namespace motts::lox;

// Not exported (internal linkage)
namespace {
    const array<pair<const char*, Token_type>, 18> reserved_words {{
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
        {"break", Token_type::break_},
        {"continue", Token_type::continue_}
    }};
}

// Exported (external linkage)
namespace motts { namespace lox {
    Token_iterator::Token_iterator(const string& source) :
        source_ {&source},
        token_begin_ {source_->cbegin()},
        token_end_ {source_->cbegin()},
        token_ {consume_token()}
    {}

    Token_iterator::Token_iterator() = default;

    Token_iterator& Token_iterator::operator++() {
        if (token_begin_ != source_->end()) {
            // We are at the beginning of the next lexeme
            token_begin_ = token_end_;
            token_ = consume_token();
        } else {
            source_ = nullptr;
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
            (!source_ && !rhs.source_) ||
            (source_ == rhs.source_ && token_begin_ == rhs.token_begin_)
        );
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

    Token Token_iterator::make_token(Token_type token_type) {
        return Token{token_type, string{token_begin_, token_end_}, {}, line_};
    }

    Token Token_iterator::make_token(Token_type token_type, Literal&& literal_value) {
        return Token{token_type, string{token_begin_, token_end_}, move(literal_value), line_};
    }

    bool Token_iterator::advance_if_match(char expected) {
        if (token_end_ != source_->end() && *token_end_ == expected) {
            ++token_end_;
            return true;
        }

        return false;
    }

    Token Token_iterator::consume_string() {
        while (token_end_ != source_->end() && *token_end_ != '"') {
            if (*token_end_ == '\n') {
                ++line_;
            }

            ++token_end_;
        }

        // Check unterminated string
        if (token_end_ == source_->end()) {
            throw Scanner_error{"Unterminated string.", line_};
        }

        // The closing "
        ++token_end_;

        // Trim surrounding quotes and normalize line endings for literal value
        string literal_value;
        copy_if(token_begin_ + 1, token_end_ - 1, back_inserter(literal_value), [] (auto c) {
            return c != '\r';
        });

        return make_token(Token_type::string, Literal{move(literal_value)});
    }

    Token Token_iterator::consume_number() {
        while (token_end_ != source_->end() && isdigit(*token_end_)) {
            ++token_end_;
        }

        // Look for a fractional part
        if (
            token_end_ != source_->end() && *token_end_ ==  '.' &&
            (token_end_ + 1) != source_->end() && isdigit(*(token_end_ + 1))
        ) {
            // Consume the "." and one digit
            token_end_ += 2;

            while (token_end_ != source_->end() && isdigit(*token_end_)) {
                ++token_end_;
            }
        }

        return make_token(Token_type::number, Literal{lexical_cast<double>(string{token_begin_, token_end_})});
    }

    Token Token_iterator::consume_identifier() {
        while (token_end_ != source_->end() && (isalnum(*token_end_) || *token_end_ == '_')) {
            ++token_end_;
        }

        const string identifier {token_begin_, token_end_};
        const auto found = find_if(reserved_words.cbegin(), reserved_words.cend(), [&] (const auto& pair) {
            return pair.first == identifier;
        });
        return make_token(found != reserved_words.cend() ? found->second : Token_type::identifier);
    }

    Token Token_iterator::consume_token() {
        // Loop because we might skip some tokens
        for (; token_end_ != source_->end(); token_begin_ = token_end_) {
            auto c = *token_end_;
            ++token_end_;

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
                        while (token_end_ != source_->end() && *token_end_ != '\n') {
                            ++token_end_;
                        }

                        continue;
                    } else {
                        return make_token(Token_type::slash);
                    }

                case '!':
                    return make_token(advance_if_match('=') ? Token_type::bang_equal : Token_type::bang);

                case '=':
                    return make_token(advance_if_match('=') ? Token_type::equal_equal : Token_type::equal);

                case '>':
                    return make_token(advance_if_match('=') ? Token_type::greater_equal : Token_type::greater);

                case '<':
                    return make_token(advance_if_match('=') ? Token_type::less_equal : Token_type::less);

                // Whitespace
                case '\n':
                    ++line_;
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
                        throw Scanner_error{"Unexpected character.", line_};
                    }
            }
        }

        // The final token is always EOF
        return Token{Token_type::eof, "", {}, line_};
    }

    Scanner_error::Scanner_error(const string& what, int line) :
        Runtime_error {"[Line " + to_string(line) + "] Error: " + what}
    {}
}}
