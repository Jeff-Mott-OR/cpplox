// Related header
#include "resolver.hpp"
// C standard headers
// C++ standard headers
#include <algorithm>
// Third-party headers
#include <gsl/gsl_util>
// This project's headers

using std::find_if;
using std::shared_ptr;
using std::string;
using std::to_string;

using gsl::finally;
using gsl::narrow;

namespace motts { namespace lox {
    Resolver::Resolver(Interpreter& interpreter) :
        interpreter_ {interpreter}
    {}

    void Resolver::visit(const shared_ptr<const Block_stmt>& stmt) {
        scopes_.push_back({});
        const auto _ = finally([&] () {
            scopes_.pop_back();
        });
        for (const auto& statement : stmt->statements) {
            statement->accept(statement, *this);
        }
    }

    void Resolver::visit(const shared_ptr<const Var_stmt>& stmt) {
        if (scopes_.size()) {
            const auto found_in_scope = find_if(
                scopes_.back().cbegin(), scopes_.back().cend(),
                [&] (const auto& var_binding) {
                    return var_binding.first == stmt->name.lexeme;
                }
            );
            if (found_in_scope != scopes_.back().cend()) {
                throw Resolver_error{"Variable with this name already declared in this scope.", stmt->name};
            }

            scopes_.back().push_back({stmt->name.lexeme, Var_binding::declared});
        }

        if (stmt->initializer) {
            stmt->initializer->accept(stmt->initializer, *this);
        }

        if (scopes_.size()) {
            find_if(scopes_.back().begin(), scopes_.back().end(), [&] (const auto& var_binding) {
                return var_binding.first == stmt->name.lexeme;
            })->second = Var_binding::defined;
        }
    }

    void Resolver::visit(const shared_ptr<const Var_expr>& expr) {
        if (scopes_.size()) {
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

        for (auto scope = scopes_.crbegin(); scope != scopes_.crend(); ++scope) {
            const auto found_in_scope = find_if(scope->cbegin(), scope->cend(), [&] (const auto& var_binding) {
                return var_binding.first == expr->name.lexeme;
            });
            if (found_in_scope != scope->cend()) {
                interpreter_.resolve(expr.get(), narrow<int>(scope - scopes_.crbegin()));
                return;
            }
        }

        // Not found; assume it is global
    }

    void Resolver::visit(const shared_ptr<const Assign_expr>& expr) {
        expr->value->accept(expr->value, *this);

        for (auto scope = scopes_.crbegin(); scope != scopes_.crend(); ++scope) {
            const auto found_in_scope = find_if(scope->cbegin(), scope->cend(), [&] (const auto& var_binding) {
                return var_binding.first == expr->name.lexeme;
            });
            if (found_in_scope != scope->cend()) {
                interpreter_.resolve(expr.get(), narrow<int>(scope - scopes_.crbegin()));
                return;
            }
        }
    }

    void Resolver::visit(const shared_ptr<const Function_stmt>& stmt) {
        if (scopes_.size()) {
            scopes_.back().push_back({stmt->name.lexeme, Var_binding::defined});
        }

        scopes_.push_back({});
        const auto _ = finally([&] () {
            scopes_.pop_back();
        });

        const auto enclosing_function_type_ = current_function_type_;
        current_function_type_ = Function_type::function;
        const auto _2 = finally([&] () {
            current_function_type_ = enclosing_function_type_;
        });

        for (const auto& param : stmt->parameters) {
            scopes_.back().push_back({param.lexeme, Var_binding::defined});
        }
        for (const auto& statement : stmt->body) {
            statement->accept(statement, *this);
        }
    }

    void Resolver::visit(const shared_ptr<const Expr_stmt>& stmt) {
        stmt->expr->accept(stmt->expr, *this);
    }

    void Resolver::visit(const shared_ptr<const If_stmt>& stmt) {
        stmt->condition->accept(stmt->condition, *this);
        stmt->then_branch->accept(stmt->then_branch, *this);
        if (stmt->else_branch) {
            stmt->else_branch->accept(stmt->else_branch, *this);
        }
    }

    void Resolver::visit(const shared_ptr<const Print_stmt>& stmt) {
        stmt->expr->accept(stmt->expr, *this);
    }

    void Resolver::visit(const shared_ptr<const Return_stmt>& stmt) {
        if (current_function_type_ == Function_type::none) {
            throw Resolver_error{"Cannot return from top-level code.", stmt->keyword};
        }

        if (stmt->value) {
            stmt->value->accept(stmt->value, *this);
        }
    }

    void Resolver::visit(const shared_ptr<const While_stmt>& stmt) {
        stmt->condition->accept(stmt->condition, *this);
        stmt->body->accept(stmt->body, *this);
    }

    void Resolver::visit(const shared_ptr<const Binary_expr>& expr) {
        expr->left->accept(expr->left, *this);
        expr->right->accept(expr->right, *this);
    }

    void Resolver::visit(const shared_ptr<const Call_expr>& expr) {
        expr->callee->accept(expr->callee, *this);
        for (const auto& argument : expr->arguments) {
            argument->accept(argument, *this);
        }
    }

    void Resolver::visit(const shared_ptr<const Grouping_expr>& expr) {
        expr->expr->accept(expr->expr, *this);
    }

    void Resolver::visit(const shared_ptr<const Literal_expr>&) {}

    void Resolver::visit(const shared_ptr<const Logical_expr>& expr) {
        expr->left->accept(expr->left, *this);
        expr->right->accept(expr->right, *this);
    }

    void Resolver::visit(const shared_ptr<const Unary_expr>& expr) {
        expr->right->accept(expr->right, *this);
    }

    Resolver_error::Resolver_error(const string& what, const Token& token) :
        Runtime_error {
            "[Line " + to_string(token.line) + "] Error '" + token.lexeme + "': " + what
        }
    {}
}}
