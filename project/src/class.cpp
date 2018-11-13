#include "class.hpp"

#include <algorithm>
#include <utility>

#include <gc.h>

#include "exception.hpp"
#include "function.hpp"
#include "interpreter.hpp"

using std::find_if;
using std::move;
using std::pair;
using std::string;
using std::vector;

namespace motts { namespace lox {
    /*
        class Class
    */

    Class::Class(const string& name, Class* superclass, vector<pair<string, Function*>>&& methods) :
        name_ {name},
        superclass_ {move(superclass)},
        methods_ {move(methods)}
    {}

    Literal Class::call(Callable* owner_this, Interpreter& interpreter, const vector<Literal>& arguments) {
        auto instance = new (GC_MALLOC(sizeof(Instance))) Instance{static_cast<Class*>(owner_this)};

        const auto found_init = find_if(methods_.cbegin(), methods_.cend(), [] (const auto& method) {
            return method.first == "init";
        });
        if (found_init != methods_.cend()) {
            const auto bound_init = found_init->second->bind(instance);
            bound_init->call(bound_init, interpreter, arguments);
        }

        return Literal{instance};
    }

    int Class::arity() const {
        const auto found_init = find_if(methods_.cbegin(), methods_.cend(), [] (const auto& method) {
            return method.first == "init";
        });

        if (found_init == methods_.cend()) {
            return 0;
        }

        return found_init->second->arity();
    }

    string Class::to_string() const {
        return name_;
    }

    Literal Class::get(Instance* instance_to_bind, const string& name) const {
        const auto found_method = find_if(methods_.cbegin(), methods_.cend(), [&] (const auto& method) {
            return method.first == name;
        });
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

    Instance::Instance(const Class* class_arg) :
        class_ {move(class_arg)}
    {}

    Literal Instance::get(Instance* owner_this, const string& name) {
        const auto found_field = find_if(fields_.cbegin(), fields_.cend(), [&] (const auto& field) {
            return field.first == name;
        });
        if (found_field != fields_.cend()) {
            return found_field->second;
        }

        return class_->get(owner_this, name);
    }

    void Instance::set(const string& name, Literal&& value) {
        const auto found = find_if(fields_.begin(), fields_.end(), [&] (const auto& field) {
            return field.first == name;
        });
        if (found != fields_.end()) {
            found->second = move(value);
        } else {
            fields_.push_back({name, move(value)});
        }
    }

    string Instance::to_string() const {
        return class_->to_string() + " instance";
    }
}}
