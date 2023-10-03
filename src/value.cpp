#include "value.hpp"

#include "object.hpp"

// Not exported (internal linkage).
namespace
{
    // Allow the internal linkage section to access names.
    using namespace motts::lox;

    struct Print_visitor
    {
        std::ostream& os;

        Print_visitor(std::ostream& os_arg)
            : os {os_arg}
        {
        }

        auto operator()(std::nullptr_t)
        {
            os << "nil";
        }

        auto operator()(bool value)
        {
            os << std::boolalpha << value;
        }

        auto operator()(double value)
        {
            os << value;
        }

        auto operator()(GC_ptr<Function> fn)
        {
            os << "<fn " << *fn->name << '>';
        }

        auto operator()(GC_ptr<Bound_method> bound_method)
        {
            (*this)(bound_method->method->function);
        }

        auto operator()(GC_ptr<Class> klass)
        {
            os << "<class " << *klass->name << '>';
        }

        auto operator()(GC_ptr<Closure> closure)
        {
            (*this)(closure->function);
        }

        auto operator()(GC_ptr<Instance> instance)
        {
            os << "<instance " << *instance->klass->name << '>';
        }

        auto operator()(GC_ptr<Native_fn>)
        {
            os << "<native fn>";
        }

        auto operator()(GC_ptr<const std::string> str)
        {
            os << *str;
        }
    };
}

namespace motts { namespace lox
{
    std::ostream& operator<<(std::ostream& os, Dynamic_type_value value)
    {
        std::visit(Print_visitor{os}, value);
        return os;
    }
}}
