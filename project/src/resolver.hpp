// Related header
// C standard headers
// C++ standard headers
#include <memory>
#include <string>
#include <utility>
#include <vector>
// Third-party headers
// This project's headers
#include "expression_impls.hpp"
#include "expression_visitor.hpp"
#include "interpreter.hpp"
#include "statement_impls.hpp"
#include "statement_visitor.hpp"
#include "token.hpp"

namespace motts { namespace lox {
    enum class Var_binding { declared, defined };
    enum class Function_type { none, function };

    class Resolver : public Expr_visitor, public Stmt_visitor {
        public:
            explicit Resolver(Interpreter&);

            void visit(const std::shared_ptr<const Block_stmt>&) override;
            void visit(const std::shared_ptr<const Var_stmt>&) override;
            void visit(const std::shared_ptr<const Var_expr>&) override;
            void visit(const std::shared_ptr<const Assign_expr>&) override;
            void visit(const std::shared_ptr<const Function_stmt>&) override;
            void visit(const std::shared_ptr<const Expr_stmt>&) override;
            void visit(const std::shared_ptr<const If_stmt>&) override;
            void visit(const std::shared_ptr<const Print_stmt>&) override;
            void visit(const std::shared_ptr<const Return_stmt>&) override;
            void visit(const std::shared_ptr<const While_stmt>&) override;
            void visit(const std::shared_ptr<const Binary_expr>&) override;
            void visit(const std::shared_ptr<const Call_expr>&) override;
            void visit(const std::shared_ptr<const Grouping_expr>&) override;
            void visit(const std::shared_ptr<const Literal_expr>&) override;
            void visit(const std::shared_ptr<const Logical_expr>&) override;
            void visit(const std::shared_ptr<const Unary_expr>&) override;

        private:
            std::vector<std::vector<std::pair<std::string, Var_binding>>> scopes_;
            Function_type current_function_type_ {Function_type::none};
            Interpreter& interpreter_;
    };

    struct Resolver_error : Runtime_error {
        using Runtime_error::Runtime_error;

        explicit Resolver_error(const std::string& what, const Token&);
    };
}}
