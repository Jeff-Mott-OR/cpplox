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

  explicit VM() {
    deferred_heap_.mark_callbacks.push_back([this] (auto do_mark) {
      // markRoots();
          for (auto slot = this->stack.begin(); slot != this->stack.end(); slot++) {
            do_mark(*slot);
          }

          for (const auto frame : this->frames) {
            do_mark(Value{frame.closure});
          }

          for (const auto& upvalue : this->open_upvalues) {
            do_mark(Value{upvalue});
          }

          // markTable(&this->globals);
              for (const auto& pair : this->globals) {
                do_mark(Value{pair.first});
                do_mark(pair.second);
              }

          markCompilerRoots();

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

InterpretResult interpret(VM&, const char* source);
Value pop();

#endif
