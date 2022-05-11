#include <stdio.h>
#include <string.h>

#include "compiler.hpp"
#include "object.hpp"
#include "value.hpp"
#include "vm.hpp"

Deferred_heap::~Deferred_heap() {
    for (const auto& ptr : deferred_ptrs) {
        delete ptr;
    }
}

void Deferred_heap::destroy(Obj* ptr) {
    bytesAllocated -= ptr->size();
    deferred_ptrs.erase(
        std::remove(deferred_ptrs.begin(), deferred_ptrs.end(), ptr),
        deferred_ptrs.end()
    );
    delete ptr;
}

void Deferred_heap::collect() {
  const auto do_mark_fn = [this] (Value value) {
    this->mark(value);
  };
  for (const auto& cb : mark_callbacks) {
    cb(do_mark_fn);
  }

  // traceReferences();
      while (!gray_ptrs.empty()) {
        const auto object = gray_ptrs.back();
        gray_ptrs.pop_back();
        object->blackenObject();
      }

  // Sweep interned strings
  for (const auto objstrptr : interned_strings_) {
    if (!objstrptr->isMarked) {
      interned_strings_.erase(objstrptr);
    }
  }

  // sweep();
      std::vector<Obj*> marked;
      std::copy_if(deferred_ptrs.begin(), deferred_ptrs.end(), std::back_inserter(marked), [&] (const auto& object) {
        return object->isMarked;
      });
      std::vector<Obj*> unreached;
      std::copy_if(deferred_ptrs.begin(), deferred_ptrs.end(), std::back_inserter(unreached), [&] (const auto& object) {
        return ! object->isMarked;
      });

      for (const auto& object : marked) {
        object->isMarked = false;
      }
      for (const auto& object : unreached) {
        destroy(object);
      }

  nextGC = bytesAllocated * /*GC_HEAP_GROW_FACTOR*/ 2;
}

void Deferred_heap::mark(Obj& object) {
    if (object.isMarked) {
        return;
    }

    object.isMarked = true;
    gray_ptrs.push_back(&object);
}

struct Mark_visitor : boost::static_visitor<void> {
    Deferred_heap& deferred_heap_;

    explicit Mark_visitor(Deferred_heap& deferred_heap_arg) :
        deferred_heap_ {deferred_heap_arg}
    {}

    // Primitives
    auto operator()(std::nullptr_t) const {}
    auto operator()(bool) const {}
    auto operator()(double) const {}

    auto operator()(ObjClass* obj) const { deferred_heap_.mark(*obj); }
    auto operator()(ObjInstance* obj) const { deferred_heap_.mark(*obj); }
    auto operator()(ObjFunction* obj) const { deferred_heap_.mark(*obj); }
    auto operator()(ObjClosure* obj) const { deferred_heap_.mark(*obj); }
    auto operator()(ObjBoundMethod* obj) const { deferred_heap_.mark(*obj); }
    auto operator()(ObjNative* obj) const { deferred_heap_.mark(*obj); }
    auto operator()(ObjString* obj) const { deferred_heap_.mark(*obj); }
    auto operator()(ObjUpvalue* obj) const { deferred_heap_.mark(*obj); }
};

void Deferred_heap::mark(Value value) {
  boost::apply_visitor(Mark_visitor{*this}, value.variant);
}


Deferred_heap deferred_heap_;

Interned_strings interned_strings_;


