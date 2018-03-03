#include "interpreter.hpp"

#include <cstddef>

#include <iostream>
#include <string>
#include <utility>

#include <boost/variant.hpp>
#include <gsl/gsl_util>

using std::cout;
using std::make_unique;
using std::move;
using std::nullptr_t;
using std::string;
using std::to_string;
using std::unique_ptr;

using boost::bad_get;
using boost::get;
using boost::static_visitor;
using gsl::finally;

// For the occasional explicit qualification, mostly just to aid us human readers
namespace lox = motts::lox;

namespace motts { namespace lox {
    Environment::Environment() = default;

    Environment::Environment(Environment& enclosing, Environment::Construct_with_enclosing)
        : enclosing_ {&enclosing}
    {}

    Environment::super_::iterator Environment::find(const std::string& var_name) {
        const auto found_own = super_::find(var_name);
        if (found_own != end()) {
            return found_own;
        }

        if (enclosing_) {
            const auto found_enclosing = enclosing_->find(var_name);
            if (found_enclosing != enclosing_->end()) {
                return found_enclosing;
            }
        }

        return end();
    }

    void Interpreter::visit(const Literal_expr& expr) {
        result_ = expr.value;
    }

    void Interpreter::visit(const Grouping_expr& expr) {
        expr.expr->accept(*this);
    }

    // False and nil are falsey, everything else is truthy
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

    void Interpreter::visit(const Unary_expr& expr) {
        const auto unary_result = lox::apply_visitor(*this, *(expr.right));

        try {
            switch (expr.op.type) {
                case Token_type::minus:
                    result_ = Literal_multi_type{ - get<double>(unary_result.value)};
                    break;

                case Token_type::bang:
                    result_ = Literal_multi_type{ ! boost::apply_visitor(Is_truthy_visitor{}, unary_result.value)};
                    break;

                default:
                    throw Interpreter_error{"Unreachable."};
            }
        } catch (const bad_get&) {
            // Convert a boost variant error into a Lox error
            throw Interpreter_error{"Unsupported operand type.", expr.op};
        }
    }

    struct Plus_visitor : static_visitor<Literal_multi_type> {
        auto operator()(const string& lhs, const string& rhs) const {
            return Literal_multi_type{lhs + rhs};
        }

        auto operator()(double lhs, double rhs) const {
            return Literal_multi_type{lhs + rhs};
        }

        // All other type combinations can't be '+'-ed together
        template<typename T, typename U>
            Literal_multi_type operator()(const T&, const U&) const {
                throw Interpreter_error{"Unsupported operand types for '+'."};
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

    void Interpreter::visit(const Binary_expr& expr) {
        const auto left_result = lox::apply_visitor(*this, *(expr.left));
        const auto right_result = lox::apply_visitor(*this, *(expr.right));

        try {
            switch (expr.op.type) {
                case Token_type::greater:
                    result_ = Literal_multi_type{get<double>(left_result.value) > get<double>(right_result.value)};
                    break;

                case Token_type::greater_equal:
                    result_ = Literal_multi_type{get<double>(left_result.value) >= get<double>(right_result.value)};
                    break;

                case Token_type::less:
                    result_ = Literal_multi_type{get<double>(left_result.value) < get<double>(right_result.value)};
                    break;

                case Token_type::less_equal:
                    result_ = Literal_multi_type{get<double>(left_result.value) <= get<double>(right_result.value)};
                    break;

                case Token_type::bang_equal:
                    result_ = Literal_multi_type{
                        !boost::apply_visitor(Is_equal_visitor{}, left_result.value, right_result.value)
                    };
                    break;

                case Token_type::equal_equal:
                    result_ = Literal_multi_type{
                        boost::apply_visitor(Is_equal_visitor{}, left_result.value, right_result.value)
                    };
                    break;

                case Token_type::minus:
                    result_ = Literal_multi_type{get<double>(left_result.value) - get<double>(right_result.value)};
                    break;

                case Token_type::plus:
                    result_ = boost::apply_visitor(Plus_visitor{}, left_result.value, right_result.value);
                    break;

                case Token_type::slash:
                    result_ = Literal_multi_type{get<double>(left_result.value) / get<double>(right_result.value)};
                    break;

                case Token_type::star:
                    result_ = Literal_multi_type{get<double>(left_result.value) * get<double>(right_result.value)};
                    break;

                default:
                    throw Interpreter_error{"Unreachable.", expr.op};
            }
        } catch (const bad_get&) {
            // Convert a boost variant error into a Lox error
            throw Interpreter_error{"Unsupported operand type.", expr.op};
        }
    }

    void Interpreter::visit(const Var_expr& expr) {
        const auto found = environment_->find(expr.name.lexeme);
        if (found == environment_->end()) {
            throw Interpreter_error{"Undefined variable '" + expr.name.lexeme + "'."};
        }

        result_ = found->second;
    }

    void Interpreter::visit(const Assign_expr& expr) {
        const auto found = environment_->find(expr.name.lexeme);
        if (found == environment_->end()) {
            throw Interpreter_error{"Undefined variable '" + expr.name.lexeme + "'."};
        }

        result_ = found->second = lox::apply_visitor(*this, *(expr.value));
    }

    void Interpreter::visit(const Logical_expr& expr) {
        auto left_result = lox::apply_visitor(*this, *(expr.left));

        // Short circuit if possible
        if (expr.op.type == Token_type::or_) {
            if (boost::apply_visitor(Is_truthy_visitor{}, left_result.value)) {
                result_ = move(left_result);
                return;
            }
        } else /* expr.op.type == Token_type::and_ */ {
            if ( ! boost::apply_visitor(Is_truthy_visitor{}, left_result.value)) {
                result_ = move(left_result);
                return;
            }
        }

        expr.right->accept(*this);
    }

    void Interpreter::visit(const Expr_stmt& stmt) {
        stmt.expr->accept(*this);
    }

    void Interpreter::visit(const If_stmt& if_stmt) {
        const auto condition_result = lox::apply_visitor(*this, *(if_stmt.condition));
        if (boost::apply_visitor(Is_truthy_visitor{}, condition_result.value)) {
            if_stmt.then_branch->accept(*this);
        } else if (if_stmt.else_branch) {
            if_stmt.else_branch->accept(*this);
        }
    }

    void Interpreter::visit(const While_stmt& stmt) {
        while (
            // IIFE so I can execute multiple statements inside while condition
            ([&] () {
                const auto condition_result = lox::apply_visitor(*this, *(stmt.condition));
                return boost::apply_visitor(Is_truthy_visitor{}, condition_result.value);
            })()
        ) {
            stmt.body->accept(*this);
        }
    }

    void Interpreter::visit(const Print_stmt& stmt) {
        cout << lox::apply_visitor(*this, *(stmt.expr)) << "\n";
    }

    void Interpreter::visit(const Var_stmt& stmt) {
        (*environment_)[stmt.name.lexeme] = (
            stmt.initializer ?
                lox::apply_visitor(*this, *(stmt.initializer)) :
                Literal_multi_type{nullptr}
        );
    }

    void Interpreter::visit(const Block_stmt& stmt) {
        auto enclosed = move(environment_);
        const auto _ = finally([&] () {
            environment_ = move(enclosed);
        });
        environment_ = make_unique<Environment>(*enclosed, Environment::Construct_with_enclosing{});

        for (const auto& statement : stmt.statements) {
            statement->accept(*this);
        }
    }

    const Literal_multi_type& Interpreter::result() const & {
        return result_;
    }

    Literal_multi_type&& Interpreter::result() && {
        return move(result_);
    }

    Interpreter_error::Interpreter_error(const string& what, const Token& token)
        : Runtime_error {
            "[Line " + to_string(token.line) + "] Error " + token.lexeme + ": " + what
        }
    {}
}}
