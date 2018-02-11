#include "token.hpp"

#include <stdexcept>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

using std::logic_error;
using std::nullptr_t;
using std::ostream;
using std::string;

using boost::apply_visitor;
using boost::is_any_of;
using boost::lexical_cast;
using boost::static_visitor;
using boost::to_upper;
using boost::trim_right_if;

namespace motts { namespace lox {
    ostream& operator<<(ostream& os, const Token_type& token_type) {
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

    struct Ostream_visitor : public static_visitor<void> {
        ostream& os;

        explicit Ostream_visitor(ostream& os_)
            : os {os_}
        {}

        // Strings and numbers serialize the same way, so let templates do the duplication
        template<typename Literal_type>
            auto operator()(const Literal_type& value) {
                os << value;
            }

        // By default, bools will serialize as "1" and "0", so specialize that case
        auto operator()(const bool& value) {
            os << (value ? "true" : "false");
        }

        // The nullptr literal is spelled "nil" in lox, so specialize that case
        auto operator()(const nullptr_t&) {
            os << "nil";
        }
    };

    ostream& operator<<(ostream& os, const Literal_multi_type& literal) {
        Ostream_visitor ostream_visitor {os};
        apply_visitor(ostream_visitor, literal.value);

        return os;
    }

    ostream& operator<<(ostream& os, const Token& token) {
        os << token.type << " " << token.lexeme;
        if (token.literal) {
            os << " " << *(token.literal);
        }

        return os;
    }
}}
