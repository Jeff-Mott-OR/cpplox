#pragma once

#include <ostream>
#include <string>

#include <boost/lexical_cast.hpp>

namespace motts { namespace lox {
    // We want to serialize an enum's name, but C++ doesn't generate that for us.
    // This will get better when C++ gets reflection and injection (https://youtu.be/4AfRAVcThyA?t=21m33s).
    // But for now settle for x-macro technique (https://en.wikipedia.org/wiki/X_Macro).
    #define MOTTS_LOX_TOKEN_TYPE_NAMES \
        /* Single char tokens */ \
        X(left_paren) X(right_paren) \
        X(left_brace) X(right_brace) \
        X(comma) X(dot) X(minus) X(plus) X(semicolon) X(slash) X(star) \
        \
        /* One or two char tokens */ \
        X(bang) X(bang_equal) X(equal) X(equal_equal) \
        X(greater) X(greater_equal) X(less) X(less_equal) \
        \
        /* Literals */ \
        X(identifier) X(string) X(number) \
        \
        /* Keywords */ \
        /* These are real reserved words in C++, so mangle their names in some way */ \
        X(t_and) X(t_class) X(t_else) X(t_false) X(t_fun) X(t_for) X(t_if) X(t_nil) X(t_or) \
        X(t_print) X(t_return) X(t_super) X(t_this) X(t_true) X(t_var) X(t_while) \
        \
        X(eof)

    enum class Token_type {
        #define X(name) name,
        MOTTS_LOX_TOKEN_TYPE_NAMES
        #undef X
    };

    std::ostream& operator<<(std::ostream& os, Token_type token_type);

    /*
    In Java, the `Token` field `Object literal` can be assigned any value of any type,
    and we can implicitly convert it to a string without any extra work on our part.
    This is possible because `Object` has an overridable `toString` method,
    and the Java library comes with pre-defined derived types for strings, doubles, bools,
    and more, whose overridden `toString` will do the right thing.

    Not so in C++. In C++, there is no universal base class from which everything inherits,
    and no pre-defined derived types with overridden `toString` methods.

    I considered Boost.Any and Boost.Variant, but I settled on templated types
    that derive from `Token` with a virtual `to_string` function, which is pretty close
    to how the Java implementation works. This lets me generate derived types automatically
    for any literal type -- current and future -- each of which overrides `to_string`
    to do the right thing for that type. This also let's me construct non literal-type tokens
    that don't contain any dummy or empty literal value.
    */

    class Token {
        public:
            Token(Token_type type, const std::string& lexeme, int line);
            virtual ~Token();
            virtual std::string to_string() const;

            // No slicing
            Token(const Token&) = delete;
            Token& operator=(const Token&) = delete;
            Token(Token&&) = delete;
            Token& operator=(Token&&) = delete;

        private:
            Token_type type_;
            std::string lexeme_;

            // Warning! This field isn't used yet in the Scanning chapter and will emit a warning
            int line_;
    };

    template<typename Literal_type>
        class Token_literal : public Token {
            public:
                Token_literal(
                    Token_type type,
                    const std::string& lexeme,
                    const Literal_type& literal_value,
                    int line
                );
                std::string to_string() const override;

            private:
                Literal_type literal_value_;
        };

    // impl Token_literal

        template<typename Literal_type>
            Token_literal<Literal_type>::Token_literal(
                Token_type type,
                const std::string& lexeme,
                const Literal_type& literal_value,
                int line
            )
                : Token {type, lexeme, line},
                  literal_value_ {literal_value}
            {}

        template<typename Literal_type>
            std::string Token_literal<Literal_type>::to_string() const {
                return Token::to_string() + " " + boost::lexical_cast<std::string>(literal_value_);
            }

    std::ostream& operator<<(std::ostream& os, const Token& token);
}}
