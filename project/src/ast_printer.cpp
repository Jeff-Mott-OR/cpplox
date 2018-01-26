#include "ast_printer.hpp"

#include <string>

#include <boost/any.hpp>

using std::string;

using boost::any;
using boost::any_cast;

namespace motts { namespace lox {
    any Ast_printer::visit_binary(const Binary_expr& expr) const {
        return "(" +
            expr.op->lexeme + " " +
            any_cast<string>(expr.left->accept(*this)) + " " +
            any_cast<string>(expr.right->accept(*this)) +
        ")";
    }

    any Ast_printer::visit_grouping(const Grouping_expr& expr) const {
        return "(group " + any_cast<string>(expr.expr->accept(*this)) + ")";
    }

    any Ast_printer::visit_literal(const Literal_expr& expr) const {
        return expr.to_string();
    }

    any Ast_printer::visit_unary(const Unary_expr& expr) const {
        return "(" +
            expr.op->lexeme + " " +
            any_cast<string>(expr.right->accept(*this)) +
        ")";
    }
}}
