#include <stdio.h>
#include <string.h>

#include "compiler.hpp"
#include "object.hpp"
#include "value.hpp"
#include "vm.hpp"

Deferred_heap::~Deferred_heap() {
    for (const auto& ptr : all_deferred_ptrs) {
        delete ptr;
    }
}

void Deferred_heap::collect() {
    // Expected side-effect: gray_worklist will be populated
    for (const auto& mark_roots_fn : mark_roots_callbacks) {
        mark_roots_fn();
    }

    while (!gray_worklist.empty()) {
        const auto gray_ptr = gray_worklist.back();
        gray_worklist.pop_back();

        // Expected side-effect: add to gray_worklist until no more refs to trace
        gray_ptr->trace_refs(*this);
    }

    // Reorder the ptrs in such a way that marked ptrs precede the not-marked ptrs.
    // - The marked ptrs will be the sequence [ all_deferred_ptrs.begin(), not_marked_begin )
    // - The not-marked ptrs will be the sequence [ not_marked_begin, all_deferred_ptrs.end() )
    const auto not_marked_begin = std::partition(
        all_deferred_ptrs.begin(), all_deferred_ptrs.end(),
        [] (const auto& gc_ptr) { return gc_ptr->is_marked; }
    );

    // Destroy
    std::for_each(not_marked_begin, all_deferred_ptrs.end(), [&] (const auto& gc_ptr) {
        // Give clients a chance to prune their own data structures
        for (const auto& on_destroy_fn : on_destroy_callbacks) {
            on_destroy_fn(gc_ptr);
        }

        delete gc_ptr;
    });
    all_deferred_ptrs.erase(not_marked_begin, all_deferred_ptrs.end());

    // Reset
    for (const auto& gc_ptr : all_deferred_ptrs) {
        gc_ptr->is_marked = false;
    }
}

struct Mark_visitor : boost::static_visitor<void> {
    Deferred_heap& deferred_heap_;

    explicit Mark_visitor(Deferred_heap& deferred_heap) :
        deferred_heap_ {deferred_heap}
    {}

    // Primitives don't need to be marked
    auto operator()(std::nullptr_t) const {}
    auto operator()(bool) const {}
    auto operator()(double) const {}

    // All other object types get marked
    template<typename ObjT>
      auto operator()(ObjT* obj) const { deferred_heap_.mark(*obj); }
};

void Deferred_heap::mark(Value value) {
  boost::apply_visitor(Mark_visitor{*this}, value.variant);
}

void Deferred_heap::mark(GC_base& object) {
    if (object.is_marked) {
        return;
    }

    object.is_marked = true;
    gray_worklist.push_back(&object);
}

// # interned.cpp

Interned_strings::Interned_strings(Deferred_heap& deferred_heap) : deferred_heap_{deferred_heap} {}

ObjString* Interned_strings::get(std::string&& new_string) {
    const auto found_interned = std::find_if(cbegin(), cend(), [&] (const auto& interned_string) {
        return new_string == interned_string->str;
    });
    if (found_interned != cend()) {
        return *found_interned;
    }

    const auto interned_string = deferred_heap_.make<ObjString>(std::move(new_string));
    insert(interned_string);

    return interned_string;
}
