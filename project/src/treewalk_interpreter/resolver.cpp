#include "resolver.hpp"

#include <algorithm>
#include <functional>

#include <gsl/gsl_util>

using std::function;
using std::string;
using std::to_string;

using gcpp::deferred_ptr;
using gsl::final_act;
using gsl::finally;
using gsl::narrow;

namespace motts { namespace lox {
    Resolver::Resolver(Interpreter& interpreter) :
        interpreter_ {interpreter}
    {}

    void Resolver::visit(const gcpp::deferred_ptr<const Block_stmt>& stmt) {
        scopes_.push_back({});
        const auto _ = finally([&] () {
            scopes_.pop_back();
        });
        for (const auto& statement : stmt->statements) {
            statement->accept(statement, *this);
        }
    }

    void Resolver::visit(const gcpp::deferred_ptr<const Class_stmt>& stmt) {
        if (!scopes_.empty()) {
            declare_var(stmt->name) = Var_binding::defined;
        }

        const auto enclosing_class_type = current_class_type_;
        current_class_type_ = Class_type::class_;
        const auto _ = finally([&] () {
            current_class_type_ = enclosing_class_type;
        });

        final_act<function<void()>> _2 {[] () {}};
        if (stmt->superclass) {
            current_class_type_ = Class_type::subclass;
            stmt->superclass->accept(stmt->superclass, *this);

            scopes_.push_back({});
            _2 = finally(function<void()>{[&] () {
                scopes_.pop_back();
            }});

            scopes_.back()["super"] = Var_binding::defined;
        }

        scopes_.push_back({});
        const auto _3 = finally([&] () {
            scopes_.pop_back();
        });
        scopes_.back()["this"] = Var_binding::defined;

        for (const auto& method : stmt->methods) {
            resolve_function(
                method->expr,
                method->expr->name->lexeme == "init" ?
                    Function_type::initializer :
                    Function_type::method
            );
        }
    }

    void Resolver::visit(const gcpp::deferred_ptr<const Var_stmt>& stmt) {
        if (!scopes_.empty()) {
            declare_var(stmt->name);
        }

        if (stmt->initializer) {
            stmt->initializer->accept(stmt->initializer, *this);
        }

        if (!scopes_.empty()) {
            scopes_.back()[stmt->name.lexeme] = Var_binding::defined;
        }
    }

    void Resolver::visit(const gcpp::deferred_ptr<const Var_expr>& expr) {
        if (!scopes_.empty()) {
            const auto found_declared_in_scope = scopes_.back().find(expr->name.lexeme);
            if (found_declared_in_scope != scopes_.back().cend() && found_declared_in_scope->second == Var_binding::declared) {
                throw Resolver_error{"Cannot read local variable in its own initializer.", expr->name};
            }
        }

        resolve_local(expr, expr->name.lexeme);
    }

    void Resolver::visit(const gcpp::deferred_ptr<const Assign_expr>& expr) {
        expr->value->accept(expr->value, *this);
        resolve_local(expr, expr->name.lexeme);
    }

    void Resolver::visit(const gcpp::deferred_ptr<const Function_stmt>& stmt) {
        if (!scopes_.empty()) {
            declare_var(*(stmt->expr->name)) = Var_binding::defined;
        }

        resolve_function(stmt->expr, Function_type::function);
    }

    void Resolver::visit(const gcpp::deferred_ptr<const Expr_stmt>& stmt) {
        stmt->expr->accept(stmt->expr, *this);
    }

    void Resolver::visit(const gcpp::deferred_ptr<const If_stmt>& stmt) {
        stmt->condition->accept(stmt->condition, *this);
        stmt->then_branch->accept(stmt->then_branch, *this);
        if (stmt->else_branch) {
            stmt->else_branch->accept(stmt->else_branch, *this);
        }
    }

    void Resolver::visit(const gcpp::deferred_ptr<const Print_stmt>& stmt) {
        stmt->expr->accept(stmt->expr, *this);
    }

    void Resolver::visit(const gcpp::deferred_ptr<const Return_stmt>& stmt) {
        if (current_function_type_ == Function_type::none) {
            throw Resolver_error{"Cannot return from top-level code.", stmt->keyword};
        }

        if (stmt->value) {
            if (current_function_type_ == Function_type::initializer) {
                throw Resolver_error{"Cannot return a value from an initializer.", stmt->keyword};
            }

            stmt->value->accept(stmt->value, *this);
        }
    }

    void Resolver::visit(const gcpp::deferred_ptr<const While_stmt>& stmt) {
        stmt->condition->accept(stmt->condition, *this);
        stmt->body->accept(stmt->body, *this);
    }

    void Resolver::visit(const gcpp::deferred_ptr<const For_stmt>& stmt) {
        stmt->condition->accept(stmt->condition, *this);
        stmt->increment->accept(stmt->increment, *this);
        stmt->body->accept(stmt->body, *this);
    }

    void Resolver::visit(const gcpp::deferred_ptr<const Break_stmt>& /*stmt*/) {}

    void Resolver::visit(const gcpp::deferred_ptr<const Continue_stmt>& /*stmt*/) {}

    void Resolver::visit(const gcpp::deferred_ptr<const Binary_expr>& expr) {
        expr->left->accept(expr->left, *this);
        expr->right->accept(expr->right, *this);
    }

    void Resolver::visit(const gcpp::deferred_ptr<const Call_expr>& expr) {
        expr->callee->accept(expr->callee, *this);
        for (const auto& argument : expr->arguments) {
            argument->accept(argument, *this);
        }
    }

    void Resolver::visit(const gcpp::deferred_ptr<const Get_expr>& expr) {
        expr->object->accept(expr->object, *this);
    }

    void Resolver::visit(const gcpp::deferred_ptr<const Set_expr>& expr) {
        expr->value->accept(expr->value, *this);
        expr->object->accept(expr->object, *this);
    }

    void Resolver::visit(const gcpp::deferred_ptr<const Super_expr>& expr) {
        if (current_class_type_ == Class_type::none) {
            throw Resolver_error{"Cannot use 'super' outside of a class.", expr->keyword};
        }
        if (current_class_type_ != Class_type::subclass) {
            throw Resolver_error{"Cannot use 'super' in a class with no superclass.", expr->keyword};
        }

        resolve_local(expr, expr->keyword.lexeme);
    }

    void Resolver::visit(const gcpp::deferred_ptr<const This_expr>& expr) {
        if (current_class_type_ == Class_type::none) {
            throw Resolver_error{"Cannot use 'this' outside of a class.", expr->keyword};
        }

        resolve_local(expr, expr->keyword.lexeme);
    }

    void Resolver::visit(const gcpp::deferred_ptr<const Function_expr>& expr) {
        resolve_function(expr, Function_type::function);
    }

    void Resolver::visit(const gcpp::deferred_ptr<const Grouping_expr>& expr) {
        expr->expr->accept(expr->expr, *this);
    }

    void Resolver::visit(const gcpp::deferred_ptr<const Literal_expr>&) {}

    void Resolver::visit(const gcpp::deferred_ptr<const Logical_expr>& expr) {
        expr->left->accept(expr->left, *this);
        expr->right->accept(expr->right, *this);
    }

    void Resolver::visit(const gcpp::deferred_ptr<const Unary_expr>& expr) {
        expr->right->accept(expr->right, *this);
    }

    Resolver::Var_binding& Resolver::declare_var(const Token& name) {
        const auto found_in_scope = scopes_.back().find(name.lexeme);
        if (found_in_scope != scopes_.back().cend()) {
            throw Resolver_error{"Variable with this name already declared in this scope.", name};
        }

        return scopes_.back()[name.lexeme] = Var_binding::declared;
    }

    void Resolver::resolve_local(const deferred_ptr<const Expr>& expr, const string& name) {
        for (auto scope = scopes_.crbegin(); scope != scopes_.crend(); ++scope) {
            const auto found_in_scope = scope->find(name);
            if (found_in_scope != scope->cend()) {
                interpreter_.resolve(expr.get(), narrow<int>(scope - scopes_.crbegin()));
                return;
            }
        }

        // Not found; assume it is global
    }

    void Resolver::resolve_function(const gcpp::deferred_ptr<const Function_expr>& expr, Function_type function_type) {
        scopes_.push_back({});
        const auto _ = finally([&] () {
            scopes_.pop_back();
        });

        const auto enclosing_function_type = current_function_type_;
        current_function_type_ = function_type;
        const auto _2 = finally([&] () {
            current_function_type_ = enclosing_function_type;
        });

        if (function_type == Function_type::function && expr->name) {
            declare_var(*(expr->name)) = Var_binding::defined;
        }
        for (const auto& param : expr->parameters) {
            declare_var(param) = Var_binding::defined;
        }
        for (const auto& statement : expr->body) {
            statement->accept(statement, *this);
        }
    }

    Resolver_error::Resolver_error(const string& what, const Token& token) :
        Runtime_error {
            "[Line " + to_string(token.line) + "] Error at '" + token.lexeme + "': " + what
        }
    {}
}}
