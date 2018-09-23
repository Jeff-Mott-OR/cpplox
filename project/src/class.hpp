#pragma once

// Related header
#include "class_fwd.hpp"
// C standard headers
// C++ standard headers
#include <string>
#include <utility>
#include <vector>
// Third-party headers
// This project's headers
#include "callable.hpp"
#include "interpreter_fwd.hpp"
#include "literal.hpp"

namespace motts { namespace lox {
    class Class : public Callable {
        public:
            Class(
                const std::string& name,
                Class* superclass,
                std::vector<std::pair<std::string, Function*>>&& methods
            );
            Literal call(
                const Callable* owner_this,
                Interpreter&,
                const std::vector<Literal>& arguments
            ) const override;
            int arity() const override;
            std::string to_string() const override;
            Literal get(Instance* instance_to_bind, const std::string& name) const;

        private:
            std::string name_;
            Class* superclass_ {};
            std::vector<std::pair<std::string, Function*>> methods_;

            friend class Instance;
    };

    class Instance {
        public:
            Instance(const Class*);
            Literal get(Instance* owner_this, const std::string& name);
            void set(const std::string& name, Literal&& value);
            std::string to_string() const;

        private:
            const Class* class_ {};
            std::vector<std::pair<std::string, Literal>> fields_;
    };
}}
