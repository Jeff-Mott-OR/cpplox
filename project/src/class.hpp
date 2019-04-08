#pragma once

#include "class_fwd.hpp"

#include <string>
#include <utility>
#include <vector>

#include "callable.hpp"
#include "function.hpp"
#include "literal.hpp"

namespace motts { namespace lox {
    class Class : public Callable {
        public:
            Class(
                gcpp::deferred_heap&,
                const std::string& name,
                const gcpp::deferred_ptr<Class>& superclass,
                const std::vector<std::pair<std::string, gcpp::deferred_ptr<Function>>>& methods
            );
            Literal call(const gcpp::deferred_ptr<Callable>& owner_this, const std::vector<Literal>& arguments) override;
            int arity() const override;
            std::string to_string() const override;
            Literal get(const gcpp::deferred_ptr<Instance>& instance_to_bind, const std::string& name) const;

        private:
            gcpp::deferred_heap& deferred_heap_;
            std::string name_;
            gcpp::deferred_ptr<Class> superclass_;
            std::vector<std::pair<std::string, gcpp::deferred_ptr<Function>>> methods_;
    };

    class Instance {
        public:
            Instance(const gcpp::deferred_ptr<Class>&);
            Literal get(const gcpp::deferred_ptr<Instance>& owner_this, const std::string& name);
            void set(const std::string& name, const Literal& value);
            std::string to_string() const;

        private:
            gcpp::deferred_ptr<Class> class_;
            std::vector<std::pair<std::string, Literal>> fields_;
    };
}}
