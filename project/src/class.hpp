#pragma once

// Related header
#include "class_fwd.hpp"
// C standard headers
// C++ standard headers
#include <memory>
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
            Class(const std::string& name, std::vector<std::pair<std::string, std::shared_ptr<Function>>>&& methods);
            Literal call(
                const std::shared_ptr<const Callable>& owner_this,
                Interpreter&,
                const std::vector<Literal>& arguments
            ) const override;
            int arity() const override;
            std::string to_string() const override;

        private:
            std::string name_;
            std::vector<std::pair<std::string, std::shared_ptr<Function>>> methods_;

            friend class Instance;
    };

    class Instance {
        public:
            Instance(std::shared_ptr<const Class>);
            Literal get(const std::shared_ptr<Instance>& owner_this, const std::string& name);
            void set(const std::string& name, Literal&& value);
            std::string to_string() const;

        private:
            std::shared_ptr<const Class> class_;
            std::vector<std::pair<std::string, Literal>> fields_;
    };
}}
