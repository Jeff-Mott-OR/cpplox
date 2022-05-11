#ifndef clox_vm_h
#define clox_vm_h

#include "object.hpp"
#include "value.hpp"
void markCompilerRoots();

#include <list>

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)

struct CallFrame {
  ObjClosure* closure;
  uint8_t* ip;
  std::vector<Value>::iterator slots;
};

Value clockNative(std::vector<Value> args);
struct VM;
extern VM* vmp;

struct VM {
  std::vector<Value> stack;
  std::vector<CallFrame> frames;
  std::unordered_map<ObjString*, Value> globals;
  std::list<ObjUpvalue*> open_upvalues;
  Deferred_heap& deferred_heap_;
  Interned_strings& interned_strings_;

  explicit VM(Deferred_heap& deferred_heap, Interned_strings& interned_strings) :
    deferred_heap_ {deferred_heap}, interned_strings_ {interned_strings}
  {
    deferred_heap_.mark_roots_callbacks.push_back([this] () {
      // markRoots();
          for (auto slot = this->stack.begin(); slot != this->stack.end(); slot++) {
            deferred_heap_.mark(*slot);
          }

          for (const auto frame : this->frames) {
            deferred_heap_.mark(Value{frame.closure});
          }

          for (const auto& upvalue : this->open_upvalues) {
            deferred_heap_.mark(Value{upvalue});
          }

          // markTable(&this->globals);
              for (const auto& pair : this->globals) {
                deferred_heap_.mark(Value{pair.first});
                deferred_heap_.mark(pair.second);
              }
    });
    deferred_heap_.on_destroy_callbacks.push_back([this] (GC_base* ptr) {
      // Most destroyed pointers won't be for an interned string, but if ptr isn't in the interned map when we call erase, then no harm done.
      interned_strings_.erase(static_cast<ObjString*>(ptr));
    });
    vmp = this;
    stack.reserve(STACK_MAX); // avoid invalidating iterators
    stack.push_back(Value{deferred_heap_.make<ObjNative>(clockNative)}); // !!! TMP GC OWNER
    globals[interned_strings_.get("clock")] = stack.back();
    stack.pop_back(); // !!! TMP GC OWNER
  }
};

typedef enum {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR
} InterpretResult;

InterpretResult interpret(VM&, const std::string& source);
Value pop();

#endif
