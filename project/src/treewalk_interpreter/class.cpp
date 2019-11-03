#include "class.hpp"

#include "function.hpp"
#include "interpreter.hpp"

using std::string;
using std::unordered_map;
using std::vector;

using gcpp::deferred_heap;
using gcpp::deferred_ptr;
using gcpp::static_pointer_cast;

namespace motts { namespace lox {
    /*
        class Class
    */

    Class::Class(
        deferred_heap& deferred_heap_arg,
        const string& name,
        const deferred_ptr<Class>& superclass,
        unordered_map<string, deferred_ptr<Function>>&& methods
    ) :
        deferred_heap_ {deferred_heap_arg},
        name_ {name},
        superclass_ {superclass},
        methods_ {move(methods)}
    {}

    Literal Class::call(const deferred_ptr<Callable>& owner_this, const vector<Literal>& arguments) {
        auto instance = deferred_heap_.make<Instance>(static_pointer_cast<Class>(owner_this));

        const auto found_init = methods_.find("init");
        if (found_init != methods_.cend()) {
            const auto bound_init = found_init->second->bind(instance);
            bound_init->call(bound_init, arguments);
        }

        return Literal{instance};
    }

    int Class::arity() const {
        const auto found_init = methods_.find("init");
        if (found_init == methods_.cend()) {
            return 0;
        }
        return found_init->second->arity();
    }

    string Class::to_string() const {
        return name_;
    }

    Literal Class::get(const deferred_ptr<Instance>& instance_to_bind, const string& name) const {
        const auto found_method = methods_.find(name);
        if (found_method != methods_.cend()) {
            return Literal{found_method->second->bind(instance_to_bind)};
        }

        if (superclass_) {
            return superclass_->get(instance_to_bind, name);
        }

        throw Runtime_error{"Undefined property '" + name + "'."};
    }

    /*
        class Instance
    */

    Instance::Instance(const deferred_ptr<Class>& class_arg) :
        class_ {class_arg}
    {}

    Literal Instance::get(const deferred_ptr<Instance>& owner_this, const string& name) {
        const auto found_field = fields_.find(name);
        if (found_field != fields_.cend()) {
            return found_field->second;
        }

        return class_->get(owner_this, name);
    }

    void Instance::set(const string& name, const Literal& value) {
        const auto found = fields_.find(name);
        if (found != fields_.end()) {
            found->second = value;
        } else {
            fields_[name] = value;
        }
    }

    string Instance::to_string() const {
        return class_->to_string() + " instance";
    }
}}
