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
#include "utility.hpp"

using std::back_inserter;
using std::chrono::duration_cast;
using std::chrono::seconds;
using std::chrono::system_clock;
using std::cout;
using std::find_if;
using std::function;
using std::make_unique;
using std::move;
using std::nullptr_t;
using std::pair;
using std::string;
using std::to_string;
using std::transform;
using std::unique_ptr;
using std::vector;

using boost::bad_get;
using boost::get;
using boost::static_visitor;
using gsl::finally;
using gsl::final_action;
using gsl::narrow;

// For the occasional explicit qualification, mostly just to aid us human readers
namespace lox = motts::lox;

namespace motts { namespace lox {
    Interpreter::Interpreter() {
        struct Clock_callable : Callable {
            Literal call(
                Callable* /*owner_this*/,
                Interpreter&,
                const vector<Literal>& /*arguments*/
            ) override {
                return Literal{narrow<double>(duration_cast<seconds>(system_clock::now().time_since_epoch()).count())};
            }

            int arity() const override {
                return 0;
            }

            string to_string() const override {
                return "<fn clock>";
            }
        };

        globals_->find_own_or_make("clock") = Literal{new (GC_MALLOC(sizeof(Clock_callable))) Clock_callable{}};
    }

    void Interpreter::visit(const Literal_expr* expr) {
        result_ = expr->value;
    }

    void Interpreter::visit(const Grouping_expr* expr) {
        expr->expr->accept(expr->expr, *this);
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

    void Interpreter::visit(const Unary_expr* expr) {
        const auto unary_result = lox::apply_visitor(*this, expr->right);

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

    void Interpreter::visit(const Binary_expr* expr) {
        const auto left_result = lox::apply_visitor(*this, expr->left);
        const auto right_result = lox::apply_visitor(*this, expr->right);

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
                        return Literal{
                            ! boost::apply_visitor(Is_equal_visitor{}, left_result.value, right_result.value)
                        };

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

    void Interpreter::visit(const Var_expr* expr) {
        result_ = lookup_variable(expr->name.lexeme, *expr)->second;
    }

    void Interpreter::visit(const Assign_expr* expr) {
        result_ = lookup_variable(expr->name.lexeme, *expr)->second = lox::apply_visitor(*this, expr->value);
    }

    void Interpreter::visit(const Logical_expr* expr) {
        auto left_result = lox::apply_visitor(*this, expr->left);

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

    struct Get_callable_visitor : static_visitor<Callable*> {
        Callable* operator()(Callable* callable) const {
            return callable;
        }

        Callable* operator()(Function* callable) const {
            return callable;
        }

        Callable* operator()(Class* callable) const {
            return callable;
        }

        // All other types are not callables
        template<typename T>
            Callable* operator()(const T&) const {
                throw Interpreter_error{"Can only call functions and classes."};
            }
    };

    void Interpreter::visit(const Call_expr* expr) {
        auto callee_result = lox::apply_visitor(*this, expr->callee);
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
                return lox::apply_visitor(*this, argument_expr);
            }
        );

        result_ = callable->call(callable, *this, arguments);
    }

    void Interpreter::visit(const Get_expr* expr) {
        const auto object_result = lox::apply_visitor(*this, expr->object);
        try {
            const auto instance = get<Instance*>(object_result.value);
            result_ = instance->get(instance, expr->name.lexeme);
        } catch (const bad_get&) {
            // Convert a boost variant error into a Lox error
            throw Interpreter_error{"Only instances have properties.", expr->name};
        }
    }

    void Interpreter::visit(const Set_expr* expr) {
        auto object_result = lox::apply_visitor(*this, expr->object);
        auto value_result = lox::apply_visitor(*this, expr->value);

        try {
            get<Instance*>(object_result.value)->set(expr->name.lexeme, Literal{value_result});
        } catch (const bad_get&) {
            // Convert a boost variant error into a Lox error
            throw Interpreter_error{"Only instances have fields.", expr->name};
        }

        result_ = move(value_result);
    }

    void Interpreter::visit(const Super_expr* expr) {
        const auto found_depth = find_if(scope_depths_.cbegin(), scope_depths_.cend(), [&] (const auto& depth) {
            return depth.first == expr;
        });
        auto superclass = get<Class*>(environment_->find_in_chain("super", found_depth->second)->second.value);
        auto instance = get<Instance*>(environment_->find_in_chain("this", found_depth->second - 1)->second.value);

        result_ = superclass->get(instance, expr->method.lexeme);
    }

    void Interpreter::visit(const This_expr* expr) {
        result_ = lookup_variable("this", *expr)->second;
    }

    void Interpreter::visit(const Function_expr* expr) {
        result_ = Literal{new (GC_MALLOC(sizeof(Function))) Function{expr, environment_}};
    }

    void Interpreter::visit(const Expr_stmt* stmt) {
        stmt->expr->accept(stmt->expr, *this);
    }

    void Interpreter::visit(const If_stmt* stmt) {
        const auto condition_result = lox::apply_visitor(*this, stmt->condition);
        if (boost::apply_visitor(Is_truthy_visitor{}, condition_result.value)) {
            stmt->then_branch->accept(stmt->then_branch, *this);
        } else if (stmt->else_branch) {
            stmt->else_branch->accept(stmt->else_branch, *this);
        }
    }

    void Interpreter::visit(const While_stmt* stmt) {
        while (
            // IIFE so I can execute multiple statements inside while condition
            ([&] () {
                const auto condition_result = lox::apply_visitor(*this, stmt->condition);
                return boost::apply_visitor(Is_truthy_visitor{}, condition_result.value);
            })()
        ) {
            stmt->body->accept(stmt->body, *this);

            if (returning_) {
                return;
            }
        }
    }

    void Interpreter::visit(const Print_stmt* stmt) {
        cout << lox::apply_visitor(*this, stmt->expr) << "\n";
    }

    void Interpreter::visit(const Var_stmt* stmt) {
        environment_->find_own_or_make(stmt->name.lexeme) = (
            stmt->initializer ?
                lox::apply_visitor(*this, stmt->initializer) :
                Literal{}
        );
    }

    void Interpreter::visit(const Block_stmt* stmt) {
        auto enclosed = move(environment_);
        environment_ = new (GC_MALLOC(sizeof(Environment))) Environment{enclosed};
        const auto _ = finally([&] () {
            environment_ = move(enclosed);
        });

        for (const auto& statement : stmt->statements) {
            statement->accept(statement, *this);

            if (returning_) {
                return;
            }
        }
    }

    void Interpreter::visit(const Class_stmt* stmt) {
        Class* superclass {};
        vector<pair<string, Function*>> methods;

        {
            Environment* enclosed {};
            if (stmt->superclass) {
                try {
                    superclass = get<Class*>(lox::apply_visitor(*this, stmt->superclass).value);
                } catch (const bad_get&) {
                    // Convert a boost variant error into a Lox error
                    throw Interpreter_error{"Superclass must be a class.", stmt->superclass->name};
                }

                enclosed = move(environment_);
                environment_ = new (GC_MALLOC(sizeof(Environment))) Environment{enclosed};
                environment_->find_own_or_make("super") = Literal{superclass};
            }
            const auto _ = finally([&] () {
                if (enclosed) {
                    environment_ = move(enclosed);
                }
            });

            for (const auto& method : stmt->methods) {
                methods.push_back({
                    method->expr->name->lexeme,
                    new (GC_MALLOC(sizeof(Function))) Function{method->expr, environment_, method->expr->name->lexeme == "init"}
                });
            }
        }

        environment_->find_own_or_make(stmt->name.lexeme) = Literal{new (GC_MALLOC(sizeof(Class))) Class{stmt->name.lexeme, move(superclass), move(methods)}};
    }

    void Interpreter::visit(const Function_stmt* stmt) {
        environment_->find_own_or_make(stmt->expr->name->lexeme) = Literal{new (GC_MALLOC(sizeof(Function))) Function{stmt->expr, environment_}};
    }

    void Interpreter::visit(const Return_stmt* stmt) {
        Literal value;
        if (stmt->value) {
            value = lox::apply_visitor(*this, stmt->value);
        }

        result_ = move(value);
        returning_ = true;
    }

    const Literal& Interpreter::result() const & {
        return result_;
    }

    Literal&& Interpreter::result() && {
        return move(result_);
    }

    unique_ptr<Resolver> Interpreter::make_resolver() {
        return make_unique<Resolver>([&] (const Expr* expr, int depth) {
            scope_depths_.push_back({expr, depth});
        });
    }

    bool Interpreter::_returning() const {
        return returning_;
    }

    Literal&& Interpreter::_return_value() && {
        returning_ = false;
        return move(result_);
    }

    final_action<function<void ()>> Interpreter::_push_environment(Environment* new_environment) {
        auto finally_pop_environment = finally(function<void ()>{[&, original_environment = move(environment_)] () {
            environment_ = move(original_environment);
        }});
        environment_ = move(new_environment);

        return finally_pop_environment;
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

    Interpreter_error::Interpreter_error(const string& what, const Token& token) :
        Runtime_error {
            "[Line " + to_string(token.line) + "] Error at '" + token.lexeme + "': " + what
        }
    {}
}}
