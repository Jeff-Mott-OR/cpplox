#include "environment.hpp"
#include <algorithm>

using std::find_if;
using std::make_pair;
using std::move;
using std::pair;
using std::string;
using std::vector;

namespace motts { namespace lox {
    Environment::Environment() = default;

    Environment::Environment(Environment* enclosed) :
        enclosed_ {move(enclosed)}
    {}

    vector<pair<string, Literal>>::iterator Environment::find_in_chain(const string& var_name) {
        const auto found_own = find_if(values_.begin(), values_.end(), [&] (const auto& value) {
            return value.first == var_name;
        });
        if (found_own != end()) {
            return found_own;
        }

        if (enclosed_) {
            const auto found_enclosed = enclosed_->find_in_chain(var_name);
            if (found_enclosed != enclosed_->end()) {
                return found_enclosed;
            }
        }

        return end();
    }

    vector<pair<string, Literal>>::iterator Environment::find_in_chain(const string& var_name, int depth) {
        auto enclosed_at_depth = this;
        for (; depth; --depth) {
            enclosed_at_depth = enclosed_at_depth->enclosed_;
        }

        const auto found_at_depth = enclosed_at_depth->find_in_chain(var_name);
        if (found_at_depth != enclosed_at_depth->end()) {
            return found_at_depth;
        }

        return end();
    }

    vector<pair<string, Literal>>::iterator Environment::end() {
        return values_.end();
    }

    Literal& Environment::find_own_or_make(const string& var_name) {
        const auto found_own = find_if(values_.begin(), values_.end(), [&] (const auto& value) {
            return value.first == var_name;
        });
        if (found_own != end()) {
            return found_own->second;
        }

        values_.push_back(make_pair(var_name, Literal{}));
        return values_.back().second;
    }
}}
