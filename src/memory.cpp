#include "memory.hpp"

#include <algorithm>

namespace motts::lox
{
    void GC_heap::mark(GC_control_block_base& control_block)
    {
        if (control_block.marked) {
            return;
        }

        control_block.marked = true;
        gray_worklist_.push_back(&control_block);
    }

    void GC_heap::collect_garbage()
    {
        // Expected side-effect: Gray worklist will be populated with roots.
        for (const auto& mark_roots_fn : on_mark_roots) {
            mark_roots_fn();
        }

        while (! gray_worklist_.empty()) {
            auto* gray_control_block = gray_worklist_.back();
            gray_worklist_.pop_back();

            // Expected side-effect: References will be marked and added to gray worklist.
            gray_control_block->trace_refs(*this);
        }
        gray_worklist_.shrink_to_fit();

        const auto not_marked_begin =
            std::partition(all_ptrs_.begin(), all_ptrs_.end(), [](const auto& control_block) { return control_block->marked; });
        std::for_each(not_marked_begin, all_ptrs_.end(), [&](const auto& control_block) {
            for (const auto& on_destroy_fn : on_destroy_ptr) {
                on_destroy_fn(*control_block);
            }
            n_allocated_bytes_ -= control_block->size();
        });
        all_ptrs_.erase(not_marked_begin, all_ptrs_.end());

        for (auto& control_block : all_ptrs_) {
            control_block->marked = false;
        }
    }

    std::size_t GC_heap::size() const
    {
        return n_allocated_bytes_;
    }
}
