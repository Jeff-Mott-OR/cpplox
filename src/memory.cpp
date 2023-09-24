#include "memory.hpp"

namespace motts { namespace lox {
    GC_heap::~GC_heap() {
        for (const auto& control_block : all_ptrs_) {
            delete control_block;
        }
    }

    void GC_heap::mark(GC_control_block_base* control_block) {
        if (control_block && ! control_block->marked) {
            gray_worklist_.push_back(control_block);
            control_block->marked = true;
        }
    }

    void GC_heap::collect_garbage() {
        // Expected side-effect: gray worklist will be populated
        for (const auto& mark_roots_fn : on_mark_roots) {
            mark_roots_fn();
        }

        while (! gray_worklist_.empty()) {
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
            delete control_block;
        });
        all_ptrs_.erase(not_marked_begin, all_ptrs_.end());

        for (const auto& control_block : all_ptrs_) {
            control_block->marked = false;
        }
    }
}}
