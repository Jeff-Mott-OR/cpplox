#include "scanner.hpp"

#include <cctype>

#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include <boost/lexical_cast.hpp>

#include "token.hpp"

using std::isalnum;
using std::isalpha;
using std::isdigit;

using std::make_unique;
using std::runtime_error;
using std::string;
using std::to_string;
using std::unique_ptr;
using std::unordered_map;
using std::vector;

using boost::lexical_cast;

using motts::lox::Token_type;

// Not exported (internal linkage)
namespace {
    // TODO: Under the microscope of profile and benchmarking tools,
    // consider changing this data structure to an array of tuples.
    // The data is small enough that an array may be faster.
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
    // class Scanner

        Scanner::Scanner(const string& source)
            : source_ {source},
              token_begin_ {source_.cbegin()},
              token_end_ {source_.cbegin()}
        {}

        const vector<unique_ptr<Token>>& Scanner::scan_tokens() {
            while (token_end_ != source_.end()) {
                // We are at the beginning of the next lexeme
                token_begin_ = token_end_;
                consume_token();
            }

            tokens_.push_back(make_unique<Token>(Token_type::eof, "", line_));

            return tokens_;
        }

        void Scanner::add_token(Token_type token_type) {
            tokens_.push_back(make_unique<Token>(
                token_type,
                string{token_begin_, token_end_},
                line_
            ));
        }

        template<typename Literal_type>
            void Scanner::add_token(Token_type token_type, const Literal_type& literal_value) {
                tokens_.push_back(make_unique<Token_literal<Literal_type>>(
                    token_type,
                    string{token_begin_, token_end_},
                    literal_value,
                    line_
                ));
            }

        bool Scanner::advance_if_match(char expected) {
            if (token_end_ != source_.end() && *token_end_ == expected) {
                ++token_end_;

                return true;
            }

            return false;
        }

        void Scanner::consume_string() {
            while (token_end_ != source_.end() && *token_end_ != '"') {
                if (*token_end_ == '\n') {
                    ++line_;
                }

                ++token_end_;
            }

            // Check unterminated string
            if (token_end_ == source_.end()) {
                throw Scanner_error{"Unterminated string.", line_};
            }

            // The closing "
            ++token_end_;

            // Trim the surrounding quotes for literal value
            add_token(Token_type::string, string{token_begin_ + 1, token_end_ - 1});
        }

        void Scanner::consume_number() {
            while (token_end_ != source_.end() && isdigit(*token_end_)) {
                ++token_end_;
            }

            // Look for a fractional part
            if (
                token_end_ != source_.end() && *token_end_ ==  '.' &&
                (token_end_ + 1) != source_.end() && isdigit(*(token_end_ + 1))
            ) {
                // Consume the "."
                ++token_end_;

                while (token_end_ != source_.end() && isdigit(*token_end_)) {
                    ++token_end_;
                }
            }

            add_token(Token_type::number, lexical_cast<double>(string{token_begin_, token_end_}));
        }

        void Scanner::consume_identifier() {
            while (token_end_ != source_.end() && (isalnum(*token_end_) || *token_end_ == '_')) {
                ++token_end_;
            }

            const auto found = reserved_words.find(string{token_begin_, token_end_});
            add_token(
                found != reserved_words.end() ?
                    found->second :
                    Token_type::identifier
            );
        }

        void Scanner::consume_token() {
            auto c = *(token_end_++);

            switch (c) {
                // Single char tokens
                case '(': add_token(Token_type::left_paren); break;
                case ')': add_token(Token_type::right_paren); break;
                case '{': add_token(Token_type::left_brace); break;
                case '}': add_token(Token_type::right_brace); break;
                case ',': add_token(Token_type::comma); break;
                case '.': add_token(Token_type::dot); break;
                case '-': add_token(Token_type::minus); break;
                case '+': add_token(Token_type::plus); break;
                case ';': add_token(Token_type::semicolon); break;
                case '*': add_token(Token_type::star); break;

                // One or two char tokens
                case '/':
                    if (advance_if_match('/')) {
                        // A comment goes until the end of the line
                        while (token_end_ != source_.end() && *token_end_ != '\n') {
                            ++token_end_;
                        }
                    } else {
                        add_token(Token_type::slash);
                    }

                    break;

                case '!':
                    add_token(
                        advance_if_match('=') ?
                            Token_type::bang_equal :
                            Token_type::bang
                    );

                    break;

                case '=':
                    add_token(
                        advance_if_match('=') ?
                            Token_type::equal_equal :
                            Token_type::equal
                    );

                    break;

                case '>':
                    add_token(
                        advance_if_match('=') ?
                            Token_type::greater_equal :
                            Token_type::greater
                    );

                    break;

                case '<':
                    add_token(
                        advance_if_match('=') ?
                            Token_type::less_equal :
                            Token_type::less
                    );

                    break;

                // Whitespace
                case '\n':
                    ++line_;

                    break;

                case ' ':
                case '\r':
                case '\t':
                    break;

                // Literals and Keywords
                case '"': consume_string(); break;
                default:
                    if (isdigit(c)) {
                        consume_number();
                    } else if (isalpha(c) || c == '_') {
                        consume_identifier();
                    } else {
                        throw Scanner_error{"Unexpected character.", line_};
                    }
            }
        }

    // class Scanner_error

        Scanner_error::Scanner_error(const string& what, int line)
            : runtime_error {"[Line " + to_string(line) + "] Error: " + what}
        {}
}}
