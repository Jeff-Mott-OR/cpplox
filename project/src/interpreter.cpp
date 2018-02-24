#include "interpreter.hpp"

#include <cstddef>

#include <string>
#include <utility>

#include <boost/variant.hpp>

using std::move;
using std::nullptr_t;
using std::string;
using std::to_string;

using boost::bad_get;
using boost::get;
using boost::static_visitor;

namespace motts { namespace lox {
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
        const auto unary_result = apply_visitor(*this, *(expr.right));

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
        const auto left_result = apply_visitor(*this, *(expr.left));
        const auto right_result = apply_visitor(*this, *(expr.right));

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
                    result_ = Literal_multi_type{ ! boost::apply_visitor(Is_equal_visitor{}, left_result.value, right_result.value)};
                    break;

                case Token_type::equal_equal:
                    result_ = Literal_multi_type{boost::apply_visitor(Is_equal_visitor{}, left_result.value, right_result.value)};
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
