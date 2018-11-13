#pragma once

#include <utility>

namespace motts { namespace lox {
    /*
    A helper function so that...

        Visitor visitor;
        expr->accept(expr, visitor);
        visitor.result()

    ...can instead be written as...

        apply_visitor<Visitor>(expr)
    */
    template<typename Visitor, typename Operand>
        auto apply_visitor(Visitor& visitor, const Operand& operand) {
            operand->accept(operand, visitor);
            return std::move(visitor).result();
        }

    template<typename Visitor, typename Operand>
        auto apply_visitor(const Operand& operand) {
            Visitor visitor;
            return apply_visitor(visitor, operand);
        }
}}
