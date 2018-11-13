#include "function.hpp"

#include <gc.h>
#include <gsl/gsl_util>

#include "interpreter.hpp"
#include "literal.hpp"

using std::move;
using std::string;
using std::vector;

using gsl::finally;
using gsl::narrow;

namespace motts { namespace lox {
    Function::Function(
        const Function_stmt* declaration,
        Environment* enclosed,
        bool is_initializer
    ) :
        declaration_ {move(declaration)},
        enclosed_ {move(enclosed)},
        is_initializer_ {is_initializer}
    {}

    Literal Function::call(
        const Callable* /*owner_this*/,
        Interpreter& interpreter,
        const vector<Literal>& arguments
    ) const {
        auto environment = new (GC_MALLOC(sizeof(Environment))) Environment{enclosed_};
        for (auto i = declaration_->parameters.cbegin(); i != declaration_->parameters.cend(); ++i) {
            environment->find_own_or_make(i->lexeme) = arguments.at(i - declaration_->parameters.cbegin());
        }

        const auto _ = interpreter._push_environment(environment);
        for (const auto& statement : declaration_->body) {
            statement->accept(statement, interpreter);

            if (interpreter._returning()) {
                return move(interpreter)._return_value();
            }
        }

        if (is_initializer_) {
            return enclosed_->find_in_chain("this")->second;
        }

        return Literal{};
    }

    int Function::arity() const {
        return narrow<int>(declaration_->parameters.size());
    }

    string Function::to_string() const {
        return "<fn " + declaration_->name.lexeme + ">";
    }

    Function* Function::bind(Instance* instance) const {
        auto this_environment = new (GC_MALLOC(sizeof(Environment))) Environment{enclosed_};
        this_environment->find_own_or_make("this") = Literal{instance};
        return new (GC_MALLOC(sizeof(Function))) Function{declaration_, this_environment, is_initializer_};
    }
}}
