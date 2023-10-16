#include "interned_strings.hpp"

namespace motts::lox
{
    Interned_strings::Interned_strings(GC_heap& gc_heap)
        : gc_heap_{gc_heap}
    {
        gc_heap_.on_destroy_ptr.push_back([this](const auto& control_block) {
            const auto maybe_gc_str_iter = strings_by_ptr_.find(&control_block);
            if (maybe_gc_str_iter != strings_by_ptr_.cend()) {
                strings_by_chars_.erase(*maybe_gc_str_iter->second);
                strings_by_ptr_.erase(maybe_gc_str_iter);
            }
        });
    }

    Interned_strings::~Interned_strings()
    {
        gc_heap_.on_destroy_ptr.pop_back();
    }

    GC_ptr<const std::string> Interned_strings::get(const char* str)
    {
        return get(std::string_view{str});
    }

    GC_ptr<const std::string> Interned_strings::get(std::string_view str)
    {
        const auto maybe_dup_iter = strings_by_chars_.find(str);
        if (maybe_dup_iter != strings_by_chars_.cend()) {
            return maybe_dup_iter->second;
        }

        const auto gc_str = gc_heap_.make<const std::string>({str.cbegin(), str.cend()});

        strings_by_chars_[*gc_str] = gc_str;
        strings_by_ptr_[gc_str.control_block] = gc_str;

        return gc_str;
    }

    GC_ptr<const std::string> Interned_strings::get(std::string&& str)
    {
        const auto maybe_dup_iter = strings_by_chars_.find(str);
        if (maybe_dup_iter != strings_by_chars_.cend()) {
            return maybe_dup_iter->second;
        }

        const auto gc_str = gc_heap_.make<const std::string>(std::move(str));

        strings_by_chars_[*gc_str] = gc_str;
        strings_by_ptr_[gc_str.control_block] = gc_str;

        return gc_str;
    }
}
