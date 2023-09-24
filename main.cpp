#include <cctype>
#include <cstdlib>
#include <iostream>
#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>
#include <gsl/gsl>

namespace program_options = boost::program_options;

namespace motts { namespace lox {
    #define MOTTS_LOX_TOKEN_TYPE_NAMES \
        X(identifier) X(number) \
        X(and_) X(or_) \
        X(eof)

    enum class Token_type {
        #define X(name) name,
        MOTTS_LOX_TOKEN_TYPE_NAMES
        #undef X
    };

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

    struct Token {
        Token_type type;
        std::string_view lexeme;
        int line;
    };

    std::ostream& operator<<(std::ostream& os, const Token& token) {
        os << "Token { "
           << "type: " << token.type << ", "
           << "lexeme: " << token.lexeme << ", "
           << "line: " << token.line
           << " }";
       return os;
    }

    class Token_iterator {
        std::string_view::const_iterator token_begin_;
        std::string_view::const_iterator token_end_;
        std::string_view::const_iterator source_end_;
        int line_ {1};
        Token token_;

        public:
            Token_iterator(std::string_view source);

            const Token& operator*() const;

        private:
            Token scan_token();
            Token scan_identifier_token();
            Token scan_number_token();
    };

    Token_iterator::Token_iterator(std::string_view source)
        : token_begin_ {source.cbegin()},
          token_end_ {source.cbegin()},
          source_end_ {source.cend()},
          token_ {scan_token()}
    {
    }

    const Token& Token_iterator::operator*() const {
        return token_;
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

            return Token{Token_type::eof, {token_begin_, token_end_}, line_};
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

    struct Lox {
        void compile(std::string_view source);
    };

    void Lox::compile(std::string_view source) {
        Token_iterator token_iter {source};
        std::cout << *token_iter << "\n";
    }

    void run_prompt(Lox& lox) {
        std::cout << "> ";

        std::string source_line;
        std::getline(std::cin, source_line);

        // If the user makes a mistake, it shouldn't kill their entire session
        try {
            lox.compile(source_line);
        } catch (const std::exception& error) {
            std::cerr << error.what() << "\n";
        }
    }
}}

std::ostream& operator<<(std::ostream& os, std::vector<std::string> vector) {
    for (const auto& string : vector) {
        std::cout << string << ' ';
    }
    return os;
}

int main(int argc, char* argv[]) {
    program_options::options_description options_description {"Usage"};
    options_description.add_options()
        ("help", "Produce help message")
        ("debug", program_options::bool_switch(), "Disassemble instructions")
        ("include-path,I", program_options::value<std::vector<std::string>>(), "Include path")
        ("input-file", program_options::value<std::vector<std::string>>(), "Input file");

    program_options::positional_options_description positional_options_description;
    positional_options_description.add("input-file", -1);

    program_options::variables_map options_map;
    program_options::store(
        program_options::command_line_parser{argc, argv}
            .options(options_description)
            .positional(positional_options_description)
            .run(),
        options_map
    );

    if (options_map.count("help")) {
        std::cout << options_description << "\n";
        return EXIT_SUCCESS;
    }
    if (options_map.count("include-path")) {
        std::cout << "Include paths are: " << options_map["include-path"].as<std::vector<std::string>>() << "\n";
    }
    if (options_map.count("input-file")) {
        std::cout << "Input files are: " << options_map["input-file"].as<std::vector<std::string>>() << "\n";
    }
    std::cout << "Debug is " << options_map["debug"].as<bool>() << "\n";

    motts::lox::Lox lox;
    if (!options_map.count("input-file")) {
        motts::lox::run_prompt(lox);
    }
}
