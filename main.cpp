#include <cstdlib>
#include <iostream>
#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <gsl/gsl>
#include <boost/program_options.hpp>

namespace program_options = boost::program_options;

namespace motts { namespace lox {
    enum class Token_type {
        eof
    };

    std::ostream& operator<<(std::ostream& os, const Token_type& type) {
        os << "EOF";
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
            const auto char_ = *token_end_++;

            return Token{Token_type::eof, {token_begin_, static_cast<unsigned long>(token_end_ - token_begin_)}, line_};
        }

        return Token{Token_type::eof, "", 0};
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
