#include "memory.hpp"

namespace motts { namespace lox {
    GC_heap::~GC_heap() {
        for (const auto& control_block : all_ptrs_) {
            delete control_block;
        }
    }

    void GC_heap::mark(GC_control_block_base* control_block) {
        if (control_block && !control_block->marked) {
            control_block->marked = true;
            gray_worklist_.push_back(control_block);
        }
    }

    std::size_t GC_heap::size() const {
        return bytes_allocated_;
    }

    void GC_heap::collect_garbage() {
        // Expected side-effect: gray worklist will be populated
        for (const auto& mark_roots_fn : on_mark_roots) {
            mark_roots_fn();
        }

        while (!gray_worklist_.empty()) {
            const auto gray_control_block = gray_worklist_.back();
            gray_worklist_.pop_back();

            // Expected side-effect: add to gray worklist until no more refs to trace
            gray_control_block->trace_refs(*this);
        }

        const auto not_marked_begin = std::partition(
            all_ptrs_.begin(), all_ptrs_.end(), [] (const auto& control_block) {
                return control_block->marked;
            }
        );

        std::for_each(not_marked_begin, all_ptrs_.end(), [&] (const auto& control_block) {
            for (const auto& on_destroy_fn : on_destroy_ptr) {
                on_destroy_fn(*control_block);
            }

            bytes_allocated_ -= control_block->size();
            delete control_block;
        });
        all_ptrs_.erase(not_marked_begin, all_ptrs_.end());

        for (const auto& control_block : all_ptrs_) {
            control_block->marked = false;
        }
    }

    Interned_strings::Interned_strings(GC_heap& gc_heap_arg) :
        gc_heap {gc_heap_arg}
    {
        gc_heap.on_destroy_ptr.push_back([&] (GC_control_block_base& control_block) {
            // Most destroyed pointers won't be for an interned string,
            // but if we call erase on a ptr that isn't in the interned set, then no harm done
            erase(GC_ptr<std::string>{static_cast<GC_control_block<std::string>*>(&control_block)});
        });
    }

    GC_ptr<std::string> Interned_strings::get(std::string&& new_string) {
        const auto found_interned_iter = std::find_if(cbegin(), cend(), [&] (const auto& string_ptr) {
            return *string_ptr == new_string;
        });
        if (found_interned_iter != cend()) {
            return *found_interned_iter;
        }

        const auto interned_string = gc_heap.make(std::move(new_string));
        insert(interned_string);

        return interned_string;
    }

    GC_ptr<std::string> Interned_strings::get(const gsl::cstring_span<>& new_string) {
        const auto found_interned_iter = std::find_if(cbegin(), cend(), [&] (const auto& string_ptr) {
            return *string_ptr == new_string;
        });
        if (found_interned_iter != cend()) {
            return *found_interned_iter;
        }

        const auto interned_string = gc_heap.make(gsl::to_string(new_string));
        insert(interned_string);

        return interned_string;
    }
}}
