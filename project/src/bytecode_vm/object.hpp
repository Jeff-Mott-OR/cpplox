#pragma once

#include "object_fwd.hpp"

#include "common.hpp"
#include "chunk.hpp"
#include "value.hpp"

#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct GC_base;

struct Deferred_heap {
    std::vector<GC_base*> all_deferred_ptrs;
    std::vector<GC_base*> gray_worklist;
    std::vector<std::function<void()>> mark_roots_callbacks;
    std::vector<std::function<void(GC_base*)>> on_destroy_callbacks;

    template<typename GC_derived, typename ...Args>
        GC_derived* make(Args... args) {
            const auto ptr = new GC_derived {std::forward<Args>(args)...};
            all_deferred_ptrs.push_back(ptr);

            mark(*ptr);
            collect();

            return ptr;
        }

    ~Deferred_heap();
    void collect();
    void mark(Value);
    void mark(GC_base&);
};

struct GC_base {
    bool is_marked {false};

    virtual ~GC_base() = default;
    virtual void trace_refs(Deferred_heap&) = 0;
    virtual std::string virtual_name() const = 0;
};

// # object.h

struct ObjString : GC_base {
  std::string str;
  explicit ObjString(std::string&& str_arg) : str{std::move(str_arg)} {}
  void trace_refs(Deferred_heap&) override {}
  std::string virtual_name() const override { return "ObjString:"+str; }
};

struct ObjFunction : GC_base {
  int arity {0};
  int upvalueCount {0};
  Chunk chunk;
  ObjString* name {nullptr};

  explicit ObjFunction(ObjString* name_arg = nullptr) : name {name_arg} {}
  void trace_refs(Deferred_heap& deferred_heap) override {
      if (name) deferred_heap.mark(*name);
      for (auto value : chunk.constants) {
        deferred_heap.mark(value);
      }
  }
  std::string virtual_name() const override { return "ObjFunction:"+(name?name->str:"<anon>"); }
};

struct ObjNative : GC_base {
  std::function<Value(std::vector<Value>)> function;
  explicit ObjNative(std::function<Value(std::vector<Value>)>&& function_arg) : function {function_arg} {}
  void trace_refs(Deferred_heap&) override {}
  std::string virtual_name() const override { return "ObjNative"; }
};

struct ObjUpvalue : GC_base {
  Value* location;
  Value closed;
  explicit ObjUpvalue(Value* location_arg) : location {location_arg} {}
  void trace_refs(Deferred_heap&) override {}
  std::string virtual_name() const override { return "ObjUpvalue"; }
};

struct ObjClosure : GC_base {
  ObjFunction* function;
  std::vector<ObjUpvalue*> upvalues;
  explicit ObjClosure(ObjFunction* function_arg) : function {function_arg} {}

  void trace_refs(Deferred_heap& deferred_heap) override {
      deferred_heap.mark(*function);
      for (const auto upvalue : upvalues) {
        deferred_heap.mark(*upvalue);
      }
  }
  std::string virtual_name() const override { return "ObjClosure:"/*+function->name->str*/; }
};

struct ObjClass : GC_base {
  ObjString* name;
  std::unordered_map<ObjString*, Value> methods;
  explicit ObjClass(ObjString* name_arg) : name {name_arg} {}

  void trace_refs(Deferred_heap& deferred_heap) override {
      deferred_heap.mark(*name);
      for (const auto& method : methods) {
        deferred_heap.mark(*method.first);
        deferred_heap.mark(method.second);
      }
  }
  std::string virtual_name() const override { return "ObjClass:"/*+name->str*/; }
};

struct ObjInstance : GC_base {
  ObjClass* klass;
  std::unordered_map<ObjString*, Value> fields; // [fields]
  explicit ObjInstance(ObjClass* klass_arg) : klass {klass_arg} {}

  void trace_refs(Deferred_heap& deferred_heap) override {
      deferred_heap.mark(*klass);
      for (const auto& pair : fields) {
        deferred_heap.mark(*pair.first);
        deferred_heap.mark(pair.second);
      }
  }
  std::string virtual_name() const override { return "ObjInstance:"/*+klass->name->str*/; }
};

struct ObjBoundMethod : GC_base {
  Value receiver;
  ObjClosure* method;
  explicit ObjBoundMethod(Value receiver_arg, ObjClosure* method_arg) : receiver {receiver_arg}, method {method_arg} {}

  void trace_refs(Deferred_heap& deferred_heap) override {
      deferred_heap.mark(receiver);
      deferred_heap.mark(*method);
  }
  std::string virtual_name() const override { return "ObjBoundMethod"; }
};

// # interned.hpp

class Interned_strings : private std::unordered_set<ObjString*> {
    Deferred_heap& deferred_heap_;

    public:
        using std::unordered_set<ObjString*>::erase;

        explicit Interned_strings(Deferred_heap& deferred_heap);

        ObjString* get(std::string&& new_string);
};
