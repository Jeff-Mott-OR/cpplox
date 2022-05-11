#ifndef clox_object_h
#define clox_object_h

#include "common.hpp"
#include "chunk.hpp"
#include "value.hpp"

#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct Obj {
    bool isMarked {false};

    virtual ~Obj() = default;
    virtual void blackenObject() {}
    virtual int size() const = 0;
};

template<typename T>
    struct ObjX : Obj {
        int size() const override {
            return sizeof(T);
        }
    };

struct Deferred_heap {
    size_t bytesAllocated = 0;
    size_t nextGC = 1024 * 1024;
    std::vector<Obj*> deferred_ptrs;
    std::vector<Obj*> gray_ptrs;

    using do_mark_fn = std::function<void(Value)>;
    std::vector<std::function<void(do_mark_fn)>> mark_callbacks;

    ~Deferred_heap();

    template<typename T, typename ...Args>
        T* make(Args... args) {
            bytesAllocated += sizeof(T);
            if (bytesAllocated > nextGC) {
                collect();
            }

            auto object = new T {std::forward<Args>(args)...};
            deferred_ptrs.push_back(object);

            return object;
        }

    void destroy(Obj*);
    void collect();
    void mark(Obj&);
    void mark(Value);
};

extern Deferred_heap deferred_heap_;

struct ObjString : ObjX<ObjString> {
  std::string str;
  explicit ObjString(std::string&& str_arg) : str{std::move(str_arg)} {}
};

struct ObjFunction : ObjX<ObjFunction> {
  int arity {0};
  int upvalueCount {0};
  Chunk chunk;
  ObjString* name {nullptr};

  void blackenObject() override {
      if (name) deferred_heap_.mark(*name);
      for (auto value : chunk.constants) {
        deferred_heap_.mark(value);
      }
  }
};

struct ObjNative : ObjX<ObjNative> {
  std::function<Value(std::vector<Value>)> function;
  explicit ObjNative(std::function<Value(std::vector<Value>)>&& function_arg) : function {function_arg} {}
};

struct ObjUpvalue : ObjX<ObjUpvalue> {
  Value* location;
  Value closed;
  explicit ObjUpvalue(Value* location_arg) : location {location_arg} {}
};

struct ObjClosure : ObjX<ObjClosure> {
  ObjFunction* function;
  std::vector<ObjUpvalue*> upvalues;
  explicit ObjClosure(ObjFunction* function_arg) : function {function_arg} {}

  void blackenObject() override {
      deferred_heap_.mark(*function);
      for (const auto upvalue : upvalues) {
        deferred_heap_.mark(*upvalue);
      }
  }
};

struct ObjClass : ObjX<ObjClass> {
  ObjString* name;
  std::unordered_map<ObjString*, Value> methods;
  explicit ObjClass(ObjString* name_arg) : name {name_arg} {}

  void blackenObject() override {
      deferred_heap_.mark(*name);
      // markTable(&methods);
          for (const auto& pair : methods) {
            deferred_heap_.mark(*pair.first);
            deferred_heap_.mark(pair.second);
          }
  }
};

struct ObjInstance : ObjX<ObjInstance> {
  ObjClass* klass;
  std::unordered_map<ObjString*, Value> fields; // [fields]
  explicit ObjInstance(ObjClass* klass_arg) : klass {klass_arg} {}

  void blackenObject() override {
      deferred_heap_.mark(*klass);
      // markTable(&fields);
          for (const auto& pair : fields) {
            deferred_heap_.mark(*pair.first);
            deferred_heap_.mark(pair.second);
          }
  }
};

struct ObjBoundMethod : ObjX<ObjBoundMethod> {
  Value receiver;
  ObjClosure* method;
  explicit ObjBoundMethod(Value receiver_arg, ObjClosure* method_arg) : receiver {receiver_arg}, method {method_arg} {}

  void blackenObject() override {
      deferred_heap_.mark(receiver);
      deferred_heap_.mark(*method);
  }
};

class Interned_strings : private std::unordered_set<ObjString*> {
    public:
        using std::unordered_set<ObjString*>::begin;
        using std::unordered_set<ObjString*>::end;
        using std::unordered_set<ObjString*>::erase;

        ObjString* get(std::string&& new_string) {
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
};

extern Interned_strings interned_strings_;

#endif
