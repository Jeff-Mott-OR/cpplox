#include "interpreter.hpp"

#include <cstddef>

#include <algorithm>
#include <chrono>
#include <iostream>
#include <iterator>

#include <boost/variant.hpp>

#include "callable.hpp"
#include "class.hpp"
#include "function.hpp"

using std::back_inserter;
using std::chrono::duration_cast;
using std::chrono::seconds;
using std::chrono::system_clock;
using std::cout;
using std::find_if;
using std::move;
using std::nullptr_t;
using std::pair;
using std::string;
using std::to_string;
using std::transform;
using std::vector;

using boost::bad_get;
using boost::get;
using boost::static_visitor;
using deferred_heap_t = gcpp::deferred_heap;
using gcpp::deferred_ptr;
using gsl::finally;
using gsl::narrow;

// Allow the internal linkage section to access names
using namespace motts::lox;

// Not exported (internal linkage)
namespace {
    /*
    A helper function so that...

        Visitor visitor;
        expr->accept(expr, visitor);
        visitor.result()

    ...can instead be written as...

        apply_visitor<Visitor>(expr)
    */
    template<typename Visitor, typename Operand>
        auto apply_visitor(Visitor& visitor, const Operand& operand) {
            operand->accept(operand, visitor);
            return std::move(visitor).result();
        }

    template<typename Visitor, typename Operand>
        auto apply_visitor(const Operand& operand) {
            Visitor visitor;
            return apply_visitor(visitor, operand);
        }

    // Only false and nil are falsey, everything else is truthy
    struct Is_truthy_visitor : static_visitor<bool> {
        auto operator()(bool value) const {
            return value;
        }

        auto operator()(nullptr_t) const {
            return false;
        }

        template<typename T>
            auto operator()(const T&) const {
                return true;
            }
    };

    struct Plus_visitor : static_visitor<Literal> {
        auto operator()(const string& lhs, const string& rhs) const {
            return Literal{lhs + rhs};
        }

        auto operator()(double lhs, double rhs) const {
            return Literal{lhs + rhs};
        }

        // All other type combinations can't be '+'-ed together
        template<typename T, typename U>
            Literal operator()(const T&, const U&) const {
                throw Interpreter_error{"Operands must be two numbers or two strings."};
            }
    };

    struct Is_equal_visitor : static_visitor<bool> {
        // Different types automatically compare false
        template<typename T, typename U>
            auto operator()(const T&, const U&) const {
                return false;
            }

        // Same types use normal equality test
        template<typename T>
            auto operator()(const T& lhs, const T& rhs) const {
                return lhs == rhs;
            }
    };

    struct Get_callable_visitor : static_visitor<deferred_ptr<Callable>> {
        deferred_ptr<Callable> operator()(const deferred_ptr<Callable>& callable) const {
            return callable;
        }

        deferred_ptr<Callable> operator()(const deferred_ptr<Function>& callable) const {
            return callable;
        }

        deferred_ptr<Callable> operator()(const deferred_ptr<Class>& callable) const {
            return callable;
        }

        // All other types are not callables
        template<typename T>
            deferred_ptr<Callable> operator()(const T&) const {
                throw Interpreter_error{"Can only call functions and classes."};
            }
    };

    class Break {};
    class Continue {};
}

// Exported (external linkage)
namespace motts { namespace lox {
    Interpreter::Interpreter(deferred_heap_t& deferred_heap) :
        deferred_heap_ {deferred_heap}
    {
        struct Clock_callable : Callable {
            Literal call(const deferred_ptr<Callable>& /*owner_this*/, const vector<Literal>& /*arguments*/) override {
                return Literal{narrow<double>(duration_cast<seconds>(system_clock::now().time_since_epoch()).count())};
            }

            int arity() const override {
                return 0;
            }

            string to_string() const override {
                return "<fn clock>";
            }
        };

        globals_->find_own_or_make("clock") = Literal{deferred_heap_.make<Clock_callable>()};
    }

    void Interpreter::visit(const deferred_ptr<const Literal_expr>& expr) {
        result_ = expr->value;
    }

    void Interpreter::visit(const deferred_ptr<const Grouping_expr>& expr) {
        expr->expr->accept(expr->expr, *this);
    }

    void Interpreter::visit(const deferred_ptr<const Unary_expr>& expr) {
        const auto unary_result = ::apply_visitor(*this, expr->right);

        result_ = ([&] () {
            try {
                switch (expr->op.type) {
                    case Token_type::minus:
                        return Literal{ - get<double>(unary_result.value)};

                    case Token_type::bang:
                        return Literal{ ! boost::apply_visitor(Is_truthy_visitor{}, unary_result.value)};

                    default:
                        throw Interpreter_error{"Unreachable.", expr->op};
                }
            } catch (const bad_get&) {
                // Convert a boost variant error into a Lox error
                throw Interpreter_error{"Operands must be numbers.", expr->op};
            }
        })();
    }

    void Interpreter::visit(const deferred_ptr<const Binary_expr>& expr) {
        const auto left_result = ::apply_visitor(*this, expr->left);
        const auto right_result = ::apply_visitor(*this, expr->right);

        result_ = ([&] () {
            try {
                switch (expr->op.type) {
                    case Token_type::greater:
                        return Literal{get<double>(left_result.value) > get<double>(right_result.value)};

                    case Token_type::greater_equal:
                        return Literal{get<double>(left_result.value) >= get<double>(right_result.value)};

                    case Token_type::less:
                        return Literal{get<double>(left_result.value) < get<double>(right_result.value)};

                    case Token_type::less_equal:
                        return Literal{get<double>(left_result.value) <= get<double>(right_result.value)};

                    case Token_type::bang_equal:
                        return Literal{ ! boost::apply_visitor(Is_equal_visitor{}, left_result.value, right_result.value)};

                    case Token_type::equal_equal:
                        return Literal{boost::apply_visitor(Is_equal_visitor{}, left_result.value, right_result.value)};

                    case Token_type::minus:
                        return Literal{get<double>(left_result.value) - get<double>(right_result.value)};

                    case Token_type::plus:
                        return boost::apply_visitor(Plus_visitor{}, left_result.value, right_result.value);

                    case Token_type::slash:
                        return Literal{get<double>(left_result.value) / get<double>(right_result.value)};

                    case Token_type::star:
                        return Literal{get<double>(left_result.value) * get<double>(right_result.value)};

                    default:
                        throw Interpreter_error{"Unreachable.", expr->op};
                }
            } catch (const bad_get&) {
                // Convert a boost variant error into a Lox error
                throw Interpreter_error{"Operands must be numbers.", expr->op};
            }
        })();
    }

    void Interpreter::visit(const deferred_ptr<const Var_expr>& expr) {
        result_ = lookup_variable(expr->name.lexeme, *expr)->second;
    }

    void Interpreter::visit(const deferred_ptr<const Assign_expr>& expr) {
        result_ = lookup_variable(expr->name.lexeme, *expr)->second = ::apply_visitor(*this, expr->value);
    }

    void Interpreter::visit(const deferred_ptr<const Logical_expr>& expr) {
        auto left_result = ::apply_visitor(*this, expr->left);

        // Short circuit if possible
        if (expr->op.type == Token_type::or_) {
            if (boost::apply_visitor(Is_truthy_visitor{}, left_result.value)) {
                result_ = move(left_result);
                return;
            }
        } else if (expr->op.type == Token_type::and_) {
            if ( ! boost::apply_visitor(Is_truthy_visitor{}, left_result.value)) {
                result_ = move(left_result);
                return;
            }
        } else {
            throw Interpreter_error{"Unreachable.", expr->op};
        }

        expr->right->accept(expr->right, *this);
    }

    void Interpreter::visit(const deferred_ptr<const Call_expr>& expr) {
        auto callee_result = ::apply_visitor(*this, expr->callee);
        const auto callable = boost::apply_visitor(Get_callable_visitor{}, callee_result.value);

        if (narrow<int>(expr->arguments.size()) != callable->arity()) {
            throw Interpreter_error{
                "Expected " + to_string(callable->arity()) +
                " arguments but got " + to_string(expr->arguments.size()) + ".",
                expr->closing_paren
            };
        }

        vector<Literal> arguments;
        transform(
            expr->arguments.cbegin(), expr->arguments.cend(), back_inserter(arguments),
            [&] (const auto& argument_expr) {
                return ::apply_visitor(*this, argument_expr);
            }
        );

        result_ = callable->call(callable, arguments);
    }

    void Interpreter::visit(const deferred_ptr<const Get_expr>& expr) {
        const auto object_result = ::apply_visitor(*this, expr->object);
        try {
            const auto instance = get<deferred_ptr<Instance>>(object_result.value);
            result_ = instance->get(instance, expr->name.lexeme);
        } catch (const bad_get&) {
            // Convert a boost variant error into a Lox error
            throw Interpreter_error{"Only instances have properties.", expr->name};
        }
    }

    void Interpreter::visit(const deferred_ptr<const Set_expr>& expr) {
        auto object_result = ::apply_visitor(*this, expr->object);
        auto value_result = ::apply_visitor(*this, expr->value);

        try {
            get<deferred_ptr<Instance>>(object_result.value)->set(expr->name.lexeme, Literal{value_result});
        } catch (const bad_get&) {
            // Convert a boost variant error into a Lox error
            throw Interpreter_error{"Only instances have fields.", expr->name};
        }

        result_ = move(value_result);
    }

    void Interpreter::visit(const deferred_ptr<const Super_expr>& expr) {
        const auto found_depth = find_if(scope_depths_.cbegin(), scope_depths_.cend(), [&] (const auto& depth) {
            return depth.first == expr.get();
        });
        auto superclass = get<deferred_ptr<Class>>(environment_->find_in_chain("super", found_depth->second)->second.value);
        auto instance = get<deferred_ptr<Instance>>(environment_->find_in_chain("this", found_depth->second - 1)->second.value);

        result_ = superclass->get(instance, expr->method.lexeme);
    }

    void Interpreter::visit(const deferred_ptr<const This_expr>& expr) {
        result_ = lookup_variable("this", *expr)->second;
    }

    void Interpreter::visit(const deferred_ptr<const Function_expr>& expr) {
        result_ = Literal{deferred_heap_.make<Function>(deferred_heap_, *this, expr, environment_)};
    }

    void Interpreter::visit(const deferred_ptr<const Expr_stmt>& stmt) {
        stmt->expr->accept(stmt->expr, *this);
    }

    void Interpreter::visit(const deferred_ptr<const If_stmt>& stmt) {
        const auto condition_result = ::apply_visitor(*this, stmt->condition);
        if (boost::apply_visitor(Is_truthy_visitor{}, condition_result.value)) {
            stmt->then_branch->accept(stmt->then_branch, *this);
        } else if (stmt->else_branch) {
            stmt->else_branch->accept(stmt->else_branch, *this);
        }
    }

    void Interpreter::visit(const deferred_ptr<const While_stmt>& stmt) {
        while (
            // IIFE so I can execute multiple statements inside while condition
            ([&] () {
                const auto condition_result = ::apply_visitor(*this, stmt->condition);
                return boost::apply_visitor(Is_truthy_visitor{}, condition_result.value);
            })()
        ) {
            try {
                stmt->body->accept(stmt->body, *this);
            } catch (const Break&) {
                break;
            } catch (const Continue&) {
                continue;
            }

            if (returning_) {
                return;
            }
        }
    }

    void Interpreter::visit(const deferred_ptr<const For_stmt>& stmt) {
        for (
            ;

            // IIFE so I can execute multiple statements inside condition
            ([&] () {
                const auto condition_result = ::apply_visitor(*this, stmt->condition);
                return boost::apply_visitor(Is_truthy_visitor{}, condition_result.value);
            })();

            ::apply_visitor(*this, stmt->increment)
        ) {
            try {
                stmt->body->accept(stmt->body, *this);
            } catch (const Break&) {
                break;
            } catch (const Continue&) {
                continue;
            }

            if (returning_) {
                return;
            }
        }
    }

    void Interpreter::visit(const deferred_ptr<const Break_stmt>&) {
        // Yes, this is using exceptions for control flow. Yes, that's usually a bad thing. In this case, exceptions
        // happen to offer exactly the behavior we need. We need to traverse an unknown call stack depth to get back to
        // the nearest loop statement.
        throw Break{};
    }

    void Interpreter::visit(const deferred_ptr<const Continue_stmt>&) {
        // Yes, this is using exceptions for control flow. Yes, that's usually a bad thing. In this case, exceptions
        // happen to offer exactly the behavior we need. We need to traverse an unknown call stack depth to get back to
        // the nearest loop statement.
        throw Continue{};
    }

    void Interpreter::visit(const deferred_ptr<const Print_stmt>& stmt) {
        cout << ::apply_visitor(*this, stmt->expr) << "\n";
    }

    void Interpreter::visit(const deferred_ptr<const Var_stmt>& stmt) {
        environment_->find_own_or_make(stmt->name.lexeme) = (
            stmt->initializer ?
                ::apply_visitor(*this, stmt->initializer) :
                Literal{}
        );
    }

    void Interpreter::visit(const deferred_ptr<const Block_stmt>& stmt) {
        execute_block(stmt->statements, deferred_heap_.make<Environment>(environment_));
    }

    void Interpreter::visit(const deferred_ptr<const Class_stmt>& stmt) {
        deferred_ptr<Class> superclass;
        deferred_ptr<Environment> method_environment {environment_};
        vector<pair<string, deferred_ptr<Function>>> methods;

        if (stmt->superclass) {
            try {
                superclass = get<deferred_ptr<Class>>(::apply_visitor(*this, stmt->superclass).value);
            } catch (const bad_get&) {
                // Convert a boost variant error into a Lox error
                throw Interpreter_error{"Superclass must be a class.", stmt->superclass->name};
            }

            method_environment = deferred_heap_.make<Environment>(environment_);
            method_environment->find_own_or_make("super") = Literal{superclass};
        }

        for (const auto& method : stmt->methods) {
            methods.push_back({
                method->expr->name->lexeme,
                deferred_heap_.make<Function>(deferred_heap_, *this, method->expr, method_environment, method->expr->name->lexeme == "init")
            });
        }

        environment_->find_own_or_make(stmt->name.lexeme) = Literal{deferred_heap_.make<Class>(deferred_heap_, stmt->name.lexeme, move(superclass), move(methods))};
    }

    void Interpreter::visit(const deferred_ptr<const Function_stmt>& stmt) {
        environment_->find_own_or_make(stmt->expr->name->lexeme) = Literal{deferred_heap_.make<Function>(deferred_heap_, *this, stmt->expr, environment_)};
    }

    void Interpreter::visit(const deferred_ptr<const Return_stmt>& stmt) {
        result_ = stmt->value ? ::apply_visitor(*this, stmt->value) : Literal{};
        returning_ = true;
    }

    const Literal& Interpreter::result() const & {
        return result_;
    }

    Literal&& Interpreter::result() && {
        return move(result_);
    }

    vector<pair<string, Literal>>::iterator Interpreter::lookup_variable(const string& name, const Expr& expr) {
        const auto found_depth = find_if(scope_depths_.cbegin(), scope_depths_.cend(), [&] (const auto& depth) {
            return depth.first == &expr;
        });
        if (found_depth != scope_depths_.cend()) {
            return environment_->find_in_chain(name, found_depth->second);
        }

        const auto found_global = globals_->find_in_chain(name);
        if (found_global != globals_->end()) {
            return found_global;
        }

        throw Interpreter_error{"Undefined variable '" + name + "'."};
    }

    void Interpreter::resolve(const Expr* expr, int depth) {
        scope_depths_.push_back({expr, depth});
    }

    void Interpreter::execute_block(const vector<deferred_ptr<const Stmt>>& statements, const deferred_ptr<Environment>& environment) {
        const auto original_environment = move(environment_);
        const auto _ = finally([&] () {
            environment_ = move(original_environment);
        });
        environment_ = environment;

        for (const auto& statement : statements) {
            statement->accept(statement, *this);

            if (returning_) {
                return;
            }
        }
    }

    bool Interpreter::returning() const {
        return returning_;
    }

    void Interpreter::returning(bool returning) {
        returning_ = returning;
    }

    Interpreter_error::Interpreter_error(const string& what, const Token& token) :
        Runtime_error {
            "[Line " + to_string(token.line) + "] Error at '" + token.lexeme + "': " + what
        }
    {}
}}
