#include "ast_printer.hpp"
#include <utility>
#include <boost/lexical_cast.hpp>

using std::move;
using std::string;

using boost::lexical_cast;
using gcpp::deferred_ptr;

namespace motts { namespace lox {
    void Ast_printer::visit(const deferred_ptr<const Binary_expr>& expr) {
        result_ += "(" + expr->op.lexeme + " ";
        expr->left->accept(expr->left, *this);
        result_ += " ";
        expr->right->accept(expr->right, *this);
        result_ += ")";
    }

    void Ast_printer::visit(const deferred_ptr<const Grouping_expr>& expr) {
        result_ += "(group ";
        expr->expr->accept(expr->expr, *this);
        result_ += ")";
    }

    void Ast_printer::visit(const deferred_ptr<const Literal_expr>& expr) {
        result_ += lexical_cast<string>(expr->value);
    }

    void Ast_printer::visit(const deferred_ptr<const Unary_expr>& expr) {
        result_ += "(" + expr->op.lexeme + " ";
        expr->right->accept(expr->right, *this);
        result_ += ")";
    }

    const string& Ast_printer::result() const & {
        return result_;
    }

    string&& Ast_printer::result() && {
        return move(result_);
    }
}}
