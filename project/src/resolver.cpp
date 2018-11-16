#include "resolver.hpp"

#include <algorithm>
#include <functional>

#include <gsl/gsl_util>

using std::find_if;
using std::function;
using std::move;
using std::pair;
using std::string;
using std::to_string;

using gsl::final_action;
using gsl::finally;
using gsl::narrow;

namespace motts { namespace lox {
    Resolver::Resolver(function<void (const Expr*, int depth)>&& on_resolve_local) :
        on_resolve_local_ {move(on_resolve_local)}
    {}

    void Resolver::visit(const Block_stmt* stmt) {
        scopes_.push_back({});
        const auto _ = finally([&] () {
            scopes_.pop_back();
        });
        for (const auto& statement : stmt->statements) {
            statement->accept(statement, *this);
        }
    }

    void Resolver::visit(const Class_stmt* stmt) {
        if (!scopes_.empty()) {
            declare_var(stmt->name).second = Var_binding::defined;
        }

        const auto enclosing_class_type = current_class_type_;
        current_class_type_ = Class_type::class_;
        const auto _ = finally([&] () {
            current_class_type_ = enclosing_class_type;
        });

        final_action<function<void ()>> _2 {[] () {}};
        if (stmt->superclass) {
            current_class_type_ = Class_type::subclass;
            stmt->superclass->accept(stmt->superclass, *this);
            scopes_.push_back({});
            _2 = finally(function<void ()>{[&] () {
                scopes_.pop_back();
            }});
            scopes_.back().push_back({"super", Var_binding::defined});
        }

        scopes_.push_back({});
        const auto _3 = finally([&] () {
            scopes_.pop_back();
        });
        scopes_.back().push_back({"this", Var_binding::defined});

        for (const auto& method : stmt->methods) {
            resolve_function(
                method->expr,
                method->expr->name->lexeme == "init" ?
                    Function_type::initializer :
                    Function_type::method
            );
        }
    }

    void Resolver::visit(const Var_stmt* stmt) {
        if (!scopes_.empty()) {
            declare_var(stmt->name);
        }

        if (stmt->initializer) {
            stmt->initializer->accept(stmt->initializer, *this);
        }

        if (!scopes_.empty()) {
            scopes_.back().back().second = Var_binding::defined;
        }
    }

    void Resolver::visit(const Var_expr* expr) {
        if (!scopes_.empty()) {
            const auto found_declared_in_scope = find_if(
                scopes_.back().cbegin(), scopes_.back().cend(),
                [&] (const auto& var_binding) {
                    return var_binding.first == expr->name.lexeme && var_binding.second == Var_binding::declared;
                }
            );
            if (found_declared_in_scope != scopes_.back().cend()) {
                throw Resolver_error{"Cannot read local variable in its own initializer.", expr->name};
            }
        }

        resolve_local(*expr, expr->name.lexeme);
    }

    void Resolver::visit(const Assign_expr* expr) {
        expr->value->accept(expr->value, *this);
        resolve_local(*expr, expr->name.lexeme);
    }

    void Resolver::visit(const Function_stmt* stmt) {
        if (!scopes_.empty()) {
            declare_var(*(stmt->expr->name)).second = Var_binding::defined;
        }

        resolve_function(stmt->expr, Function_type::function);
    }

    void Resolver::visit(const Expr_stmt* stmt) {
        stmt->expr->accept(stmt->expr, *this);
    }

    void Resolver::visit(const If_stmt* stmt) {
        stmt->condition->accept(stmt->condition, *this);
        stmt->then_branch->accept(stmt->then_branch, *this);
        if (stmt->else_branch) {
            stmt->else_branch->accept(stmt->else_branch, *this);
        }
    }

    void Resolver::visit(const Print_stmt* stmt) {
        stmt->expr->accept(stmt->expr, *this);
    }

    void Resolver::visit(const Return_stmt* stmt) {
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

    void Resolver::visit(const While_stmt* stmt) {
        stmt->condition->accept(stmt->condition, *this);
        stmt->body->accept(stmt->body, *this);
    }

    void Resolver::visit(const For_stmt* stmt) {
        stmt->condition->accept(stmt->condition, *this);
        stmt->increment->accept(stmt->increment, *this);
        stmt->body->accept(stmt->body, *this);
    }

    void Resolver::visit(const Break_stmt* stmt) {}

    void Resolver::visit(const Continue_stmt* stmt) {}

    void Resolver::visit(const Binary_expr* expr) {
        expr->left->accept(expr->left, *this);
        expr->right->accept(expr->right, *this);
    }

    void Resolver::visit(const Call_expr* expr) {
        expr->callee->accept(expr->callee, *this);
        for (const auto& argument : expr->arguments) {
            argument->accept(argument, *this);
        }
    }

    void Resolver::visit(const Get_expr* expr) {
        expr->object->accept(expr->object, *this);
    }

    void Resolver::visit(const Set_expr* expr) {
        expr->value->accept(expr->value, *this);
        expr->object->accept(expr->object, *this);
    }

    void Resolver::visit(const Super_expr* expr) {
        if (current_class_type_ == Class_type::none) {
            throw Resolver_error{"Cannot use 'super' outside of a class.", expr->keyword};
        }
        if (current_class_type_ != Class_type::subclass) {
            throw Resolver_error{"Cannot use 'super' in a class with no superclass.", expr->keyword};
        }

        resolve_local(*expr, expr->keyword.lexeme);
    }

    void Resolver::visit(const This_expr* expr) {
        if (current_class_type_ == Class_type::none) {
            throw Resolver_error{"Cannot use 'this' outside of a class.", expr->keyword};
        }

        resolve_local(*expr, expr->keyword.lexeme);
    }

    void Resolver::visit(const Function_expr* expr) {
        resolve_function(expr, Function_type::function);
    }

    void Resolver::visit(const Grouping_expr* expr) {
        expr->expr->accept(expr->expr, *this);
    }

    void Resolver::visit(const Literal_expr*) {}

    void Resolver::visit(const Logical_expr* expr) {
        expr->left->accept(expr->left, *this);
        expr->right->accept(expr->right, *this);
    }

    void Resolver::visit(const Unary_expr* expr) {
        expr->right->accept(expr->right, *this);
    }

    pair<string, Var_binding>& Resolver::declare_var(const Token& name) {
        const auto found_in_scope = find_if(
            scopes_.back().cbegin(), scopes_.back().cend(),
            [&] (const auto& var_binding) {
                return var_binding.first == name.lexeme;
            }
        );
        if (found_in_scope != scopes_.back().cend()) {
            throw Resolver_error{"Variable with this name already declared in this scope.", name};
        }

        scopes_.back().push_back({name.lexeme, Var_binding::declared});

        return scopes_.back().back();
    }

    void Resolver::resolve_local(const Expr& expr, const string& name) {
        for (auto scope = scopes_.crbegin(); scope != scopes_.crend(); ++scope) {
            const auto found_in_scope = find_if(scope->cbegin(), scope->cend(), [&] (const auto& var_binding) {
                return var_binding.first == name;
            });
            if (found_in_scope != scope->cend()) {
                on_resolve_local_(&expr, narrow<int>(scope - scopes_.crbegin()));
                return;
            }
        }

        // Not found; assume it is global
    }

    void Resolver::resolve_function(const Function_expr* expr, Function_type function_type) {
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
            declare_var(*(expr->name)).second = Var_binding::defined;
        }
        for (const auto& param : expr->parameters) {
            declare_var(param).second = Var_binding::defined;
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
