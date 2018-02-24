#include "ast_printer.hpp"

#include <utility>

#include <boost/lexical_cast.hpp>

using std::move;
using std::string;

using boost::lexical_cast;

namespace motts { namespace lox {
    void Ast_printer::visit(const Binary_expr& expr) {
        result_ += "(" + expr.op.lexeme + " ";
        expr.left->accept(*this);
        result_ += " ";
        expr.right->accept(*this);
        result_ += ")";
    }

    void Ast_printer::visit(const Grouping_expr& expr) {
        result_ += "(group ";
        expr.expr->accept(*this);
        result_ += ")";
    }

    void Ast_printer::visit(const Literal_expr& expr) {
        result_ += lexical_cast<string>(expr.value);
    }

    void Ast_printer::visit(const Unary_expr& expr) {
        result_ += "(" + expr.op.lexeme + " ";
        expr.right->accept(*this);
        result_ += ")";
    }

    const string& Ast_printer::result() const & {
        return result_;
    }

    string&& Ast_printer::result() && {
        return move(result_);
    }
}}
