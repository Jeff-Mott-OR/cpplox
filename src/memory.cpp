#include "memory.hpp"

#include <algorithm>

namespace motts { namespace lox {
    GC_heap::~GC_heap() {
        for (const auto* control_block : all_ptrs_) {
            delete control_block;
        }
    }

    void GC_heap::mark(GC_control_block_base* control_block) {
        if (control_block && ! control_block->marked) {
            control_block->marked = true;
            gray_worklist_.push_back(control_block);
        }
    }

    void GC_heap::collect_garbage() {
        // Expected side-effect: Gray worklist will be populated with roots.
        for (const auto& mark_roots_fn : on_mark_roots) {
            mark_roots_fn();
        }

        while (! gray_worklist_.empty()) {
            auto* gray_control_block = gray_worklist_.back();
            gray_worklist_.pop_back();

            // Expected side-effect: Traced references will be marked and added to gray worklist.
            gray_control_block->trace_refs(*this);
        }

        const auto not_marked_begin = std::partition(
            all_ptrs_.begin(), all_ptrs_.end(),
            [] (const auto* control_block) {
                return control_block->marked;
            }
        );

        std::for_each(not_marked_begin, all_ptrs_.end(), [] (const auto* control_block) {
            delete control_block;
        });
        all_ptrs_.erase(not_marked_begin, all_ptrs_.end());

        for (auto* control_block : all_ptrs_) {
            control_block->marked = false;
        }
    }
}}
