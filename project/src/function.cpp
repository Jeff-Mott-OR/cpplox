#include "function.hpp"
#include <gsl/gsl_util>
#include "literal.hpp"

using std::move;
using std::string;
using std::vector;

using deferred_heap_t = gcpp::deferred_heap;
using gcpp::deferred_ptr;
using gsl::finally;
using gsl::narrow;

namespace motts { namespace lox {
    Function::Function(
        deferred_heap_t& deferred_heap,
        Interpreter& interpreter,
        const deferred_ptr<const Function_expr>& declaration,
        const deferred_ptr<Environment>& enclosed,
        bool is_initializer
    ) :
        deferred_heap_ {deferred_heap},
        interpreter_ {interpreter},
        declaration_ {declaration},
        enclosed_ {enclosed},
        is_initializer_ {is_initializer}
    {}

    Literal Function::call(const deferred_ptr<Callable>& owner_this, const vector<Literal>& arguments) {
        auto environment = deferred_heap_.make<Environment>(enclosed_);
        if (declaration_->name) {
            environment->find_own_or_make(declaration_->name->lexeme) = Literal{owner_this};
        }
        for (auto param_iter = declaration_->parameters.cbegin(); param_iter != declaration_->parameters.cend(); ++param_iter) {
            environment->find_own_or_make(param_iter->lexeme) = arguments.at(param_iter - declaration_->parameters.cbegin());
        }

        interpreter_.execute_block(declaration_->body, environment);

        if (interpreter_.returning()) {
            interpreter_.returning(false);
            return move(interpreter_).result();
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
        return "<fn " + (declaration_->name ? declaration_->name->lexeme : "[[anonymous]]") + ">";
    }

    deferred_ptr<Function> Function::bind(const deferred_ptr<Instance>& instance) const {
        auto this_environment = deferred_heap_.make<Environment>(enclosed_);
        this_environment->find_own_or_make("this") = Literal{instance};
        return deferred_heap_.make<Function>(deferred_heap_, interpreter_, declaration_, this_environment, is_initializer_);
    }
}}
