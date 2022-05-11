#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <iostream>

#include "common.hpp"
#include "compiler.hpp"
#include "debug.hpp"
#include "object.hpp"
#include "vm.hpp"

#include <chrono>
#include <gsl/gsl_util>

VM* vmp;

Value clockNative(std::vector<Value> /*args*/) {
   return Value{gsl::narrow<double>(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count())};
}

static void runtimeError(const char* format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputs("\n", stderr);

  for (auto i = vmp->frames.rbegin(); i != vmp->frames.rend(); ++i) {
    CallFrame* frame = &*i;
    ObjFunction* function = frame->closure->function;
    // -1 because the IP is sitting on the next instruction to be
    // executed.
    size_t instruction = frame->ip - function->chunk.code.data() - 1;
    fprintf(stderr, "[line %d] in ",
            function->chunk.lines.at(instruction));
    if (function->name == NULL) {
      fprintf(stderr, "script\n");
    } else {
      fprintf(stderr, "%s()\n", function->name->str.c_str());
    }
  }
}

Value pop() {
  const auto back = vmp->stack.back();
  vmp->stack.pop_back();
  return back;
}
static Value& peek(int distance) {
  return *(vmp->stack.end() - 1 - distance);
}
static bool call(ObjClosure* closure, int argCount) {
  if (argCount != closure->function->arity) {
    runtimeError("Expected %d arguments but got %d.",
        closure->function->arity, argCount);
    return false;
  }

  if (vmp->frames.size() == FRAMES_MAX) {
    runtimeError("Stack overflow.");
    return false;
  }

  vmp->frames.push_back({closure, closure->function->chunk.code.data(), vmp->stack.end() - argCount - 1});

  return true;
}

namespace {
    struct Is_string_visitor : boost::static_visitor<bool> {
        auto operator()(ObjString*) const {
            return true;
        }

        template<typename T>
            auto operator()(const T&) const {
               return false;
            }
    };

    struct Is_number_visitor : boost::static_visitor<bool> {
        auto operator()(double) const {
            return true;
        }

        template<typename T>
            auto operator()(const T&) const {
               return false;
            }
    };

    struct Is_class_visitor : boost::static_visitor<bool> {
        auto operator()(ObjClass*) const {
            return true;
        }

        template<typename T>
            auto operator()(const T&) const {
               return false;
            }
    };

    struct Is_instance_visitor : boost::static_visitor<bool> {
        auto operator()(ObjInstance*) const {
            return true;
        }

        template<typename T>
            auto operator()(const T&) const {
               return false;
            }
    };

    struct Is_truthy_visitor : boost::static_visitor<bool> {
        auto operator()(std::nullptr_t) const {
            return false;
        }

        auto operator()(bool value) const {
            return value;
        }

        template<typename T>
            auto operator()(const T&) const {
               return true;
            }
    };

    auto is_falsey(const Value& value) {
      return ! boost::apply_visitor(Is_truthy_visitor{}, value.variant);
    }

    struct Call_visitor : boost::static_visitor<bool> {
        int argCount;
        explicit Call_visitor(int argCount_arg) : argCount{argCount_arg} {}

        auto operator()(ObjBoundMethod* bound) const {
            *(vmp->stack.end() - argCount - 1) = bound->receiver;
            return call(bound->method, argCount);
        }

        auto operator()(ObjClass* klass) const {
            *(vmp->stack.end() - argCount - 1) = Value{deferred_heap_.make<ObjInstance>(klass)};
            const auto initializer_iter = klass->methods.find(interned_strings_.get("init"));
            if (initializer_iter != klass->methods.end()) {
              return call(boost::get<ObjClosure*>(initializer_iter->second.variant), argCount);
            } else if (argCount != 0) {
              runtimeError("Expected 0 arguments but got %d.", argCount);
              return false;
            }

            return true;
        }

        auto operator()(ObjClosure* callable) const {
            return call(callable, argCount);
        }

        auto operator()(ObjNative* native) const {
            Value result = native->function({vmp->stack.end() - argCount, vmp->stack.end()});
            vmp->stack.erase(vmp->stack.end() - argCount - 1, vmp->stack.end());
            vmp->stack.push_back(result);
            return true;
        }

        template<typename T>
          auto operator()(const T&) const {
              runtimeError("Can only call functions and classes.");
              return false;
          }
    };

    bool call_value(Value callee, int argCount) {
        Call_visitor call_visitor {argCount};
        return boost::apply_visitor(call_visitor, callee.variant);
    }
}

static bool invokeFromClass(ObjClass* klass, ObjString* name,
                            int argCount) {
  const auto method_iter = klass->methods.find(name);
  if (method_iter == klass->methods.end()) {
    runtimeError("Undefined property '%s'.", name->str.c_str());
    return false;
  }

  return call(boost::get<ObjClosure*>(method_iter->second.variant), argCount);
}
static bool invoke(ObjString* name, int argCount) {
  Value receiver = peek(argCount);

  if (!boost::apply_visitor(Is_instance_visitor{}, receiver.variant)) {
    runtimeError("Only instances have methods.");
    return false;
  }

  ObjInstance* instance = boost::get<ObjInstance*>(receiver.variant);

  const auto value_iter = instance->fields.find(name);
  if (value_iter != instance->fields.end()) {
    *(vmp->stack.end() - argCount - 1) = value_iter->second;
    return call_value(value_iter->second, argCount);
  }

  return invokeFromClass(instance->klass, name, argCount);
}
static bool bindMethod(ObjClass* klass, ObjString* name) {

  const auto method_iter = klass->methods.find(name);
  if (method_iter == klass->methods.end()) {
    runtimeError("Undefined property '%s'.", name->str.c_str());
    return false;
  }

  ObjBoundMethod* bound = deferred_heap_.make<ObjBoundMethod>(peek(0), boost::get<ObjClosure*>(method_iter->second.variant));
  pop();
  vmp->stack.push_back(Value{bound});
  return true;
}
static ObjUpvalue* captureUpvalue(Value* local) {
  const auto upvalue_iter = std::find_if(vmp->open_upvalues.begin(), vmp->open_upvalues.end(), [&] (const auto& upvalue) {
    return upvalue->location == local;
  });
  if (upvalue_iter != vmp->open_upvalues.end()) return *upvalue_iter;

  ObjUpvalue* createdUpvalue = deferred_heap_.make<ObjUpvalue>(local);

  const auto upvalue_insert_iter = std::find_if(vmp->open_upvalues.begin(), vmp->open_upvalues.end(), [&] (const auto& upvalue) {
    return upvalue->location < local;
  });
  if (upvalue_insert_iter != vmp->open_upvalues.end()) {
    vmp->open_upvalues.insert(upvalue_insert_iter, createdUpvalue);
  } else {
    vmp->open_upvalues.push_back(createdUpvalue);
  }

  return createdUpvalue;
}
static void closeUpvalues(Value* last) {
  while (!vmp->open_upvalues.empty() && vmp->open_upvalues.front()->location >= last) {
    ObjUpvalue* upvalue = vmp->open_upvalues.front();
    upvalue->closed = *upvalue->location;
    upvalue->location = &upvalue->closed;
    vmp->open_upvalues.pop_front();
  }
}
static void defineMethod(ObjString* name) {
  Value method = peek(0);
  ObjClass* klass = boost::get<ObjClass*>((peek(1).variant));
  klass->methods[name] = method;
  pop();
}

static void concatenate() {
  ObjString* b = boost::get<ObjString*>((peek(0).variant));
  ObjString* a = boost::get<ObjString*>((peek(1).variant));

  ObjString* result = interned_strings_.get(a->str + b->str);

  pop();
  pop();
  vmp->stack.push_back(Value{result});
}

static InterpretResult run() {
  CallFrame* frame = &vmp->frames.back();

#define READ_BYTE() (*frame->ip++)
#define READ_SHORT() \
    (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))
#define READ_CONSTANT() \
    (frame->closure->function->chunk.constants.at(READ_BYTE()))
#define READ_STRING() boost::get<ObjString*>((READ_CONSTANT().variant))

#define BINARY_OP(op) \
    do { \
      if (! boost::apply_visitor(Is_number_visitor{}, (peek(0)).variant) || ! boost::apply_visitor(Is_number_visitor{}, (peek(1)).variant)) { \
        runtimeError("Operands must be numbers."); \
        return INTERPRET_RUNTIME_ERROR; \
      } \
      double b = boost::get<double>((pop()).variant); \
      double a = boost::get<double>((pop()).variant); \
      vmp->stack.push_back(Value{a op b}); \
    } while (false)

  for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
    printf("          ");
    for (auto slot = vmp->stack.begin(); slot != vmp->stack.end(); slot++) {
      std::cout << "[ " << *slot << " ]";
    }
    printf("\n");
    disassembleInstruction(&frame->closure->function->chunk,
        (int)(frame->ip - frame->closure->function->chunk.code.data()));
#endif

    auto instruction = static_cast<Op_code>(READ_BYTE());
    switch (instruction) {
      case Op_code::constant_: {
        Value constant = READ_CONSTANT();
        vmp->stack.push_back(constant);
        break;
      }
      case Op_code::nil_: vmp->stack.push_back(Value{nullptr}); break;
      case Op_code::true_: vmp->stack.push_back(Value{true}); break;
      case Op_code::false_: vmp->stack.push_back(Value{false}); break;
      case Op_code::pop_: pop(); break;

      case Op_code::get_local_: {
        uint8_t slot = READ_BYTE();
        vmp->stack.push_back(*(frame->slots + slot));
        break;
      }

      case Op_code::set_local_: {
        uint8_t slot = READ_BYTE();
        *(frame->slots + slot) = peek(0);
        break;
      }

      case Op_code::get_global_: {
        ObjString* name = READ_STRING();
        const auto value_iter = vmp->globals.find(name);
        if (value_iter == vmp->globals.end()) {
          runtimeError("Undefined variable '%s'.", name->str.c_str());
          return INTERPRET_RUNTIME_ERROR;
        }
        vmp->stack.push_back(value_iter->second);
        break;
      }

      case Op_code::define_global_: {
        ObjString* name = READ_STRING();
        vmp->globals[name] = peek(0);
        pop();
        break;
      }

      case Op_code::set_global_: {
        ObjString* name = READ_STRING();
        const auto value_iter = vmp->globals.find(name);
        if (value_iter == vmp->globals.end()) {
          runtimeError("Undefined variable '%s'.", name->str.c_str());
          return INTERPRET_RUNTIME_ERROR;
        }
        vmp->globals[name] = peek(0);
        break;
      }

      case Op_code::get_upvalue_: {
        uint8_t slot = READ_BYTE();
        vmp->stack.push_back(*frame->closure->upvalues.at(slot)->location);
        break;
      }

      case Op_code::set_upvalue_: {
        uint8_t slot = READ_BYTE();
        *frame->closure->upvalues.at(slot)->location = peek(0);
        break;
      }

      case Op_code::get_property_: {
        if (!boost::apply_visitor(Is_instance_visitor{}, (peek(0)).variant)) {
          runtimeError("Only instances have properties.");
          return INTERPRET_RUNTIME_ERROR;
        }

        ObjInstance* instance = boost::get<ObjInstance*>((peek(0).variant));
        ObjString* name = READ_STRING();

        const auto value_iter = instance->fields.find(name);
        if (value_iter != instance->fields.end()) {
          pop(); // Instance.
          vmp->stack.push_back(value_iter->second);
          break;
        }

        if (!bindMethod(instance->klass, name)) {
          return INTERPRET_RUNTIME_ERROR;
        }
        break;
      }

      case Op_code::set_property_: {
        if (!boost::apply_visitor(Is_instance_visitor{}, (peek(1)).variant)) {
          runtimeError("Only instances have fields.");
          return INTERPRET_RUNTIME_ERROR;
        }

        ObjInstance* instance = boost::get<ObjInstance*>((peek(1).variant));
        instance->fields[READ_STRING()] = peek(0);

        Value value = pop();
        pop();
        vmp->stack.push_back(value);
        break;
      }

      case Op_code::get_super_: {
        ObjString* name = READ_STRING();
        ObjClass* superclass = boost::get<ObjClass*>((pop().variant));
        if (!bindMethod(superclass, name)) {
          return INTERPRET_RUNTIME_ERROR;
        }
        break;
      }

      case Op_code::equal_: {
        Value b = pop();
        Value a = pop();
        vmp->stack.push_back(Value{a == b});
        break;
      }

      case Op_code::greater_:  BINARY_OP(>); break;
      case Op_code::less_:     BINARY_OP(<); break;
      case Op_code::add_: {
        if (boost::apply_visitor(Is_string_visitor{}, (peek(0)).variant) && boost::apply_visitor(Is_string_visitor{}, (peek(1)).variant)) {
          concatenate();
        } else if (boost::apply_visitor(Is_number_visitor{}, (peek(0)).variant) && boost::apply_visitor(Is_number_visitor{}, (peek(1)).variant)) {
          double b = boost::get<double>((pop()).variant);
          double a = boost::get<double>((pop()).variant);
          vmp->stack.push_back(Value{a + b});
        } else {
          runtimeError("Operands must be two numbers or two strings.");
          return INTERPRET_RUNTIME_ERROR;
        }
        break;
      }
      case Op_code::subtract_: BINARY_OP(-); break;
      case Op_code::multiply_: BINARY_OP(*); break;
      case Op_code::divide_:   BINARY_OP(/); break;
      case Op_code::not_:
        vmp->stack.push_back(Value{is_falsey(pop())});
        break;
      case Op_code::negate_:
        if (! boost::apply_visitor(Is_number_visitor{}, (peek(0)).variant)) {
          runtimeError("Operand must be a number.");
          return INTERPRET_RUNTIME_ERROR;
        }

        vmp->stack.push_back(Value{-boost::get<double>((pop()).variant)});
        break;

      case Op_code::print_: {
        std::cout << pop() << "\n";
        break;
      }

      case Op_code::jump_: {
        uint16_t offset = READ_SHORT();
        frame->ip += offset;
        break;
      }

      case Op_code::jump_if_false_: {
        uint16_t offset = READ_SHORT();
        if (is_falsey(peek(0))) frame->ip += offset;
        break;
      }

      case Op_code::loop_: {
        uint16_t offset = READ_SHORT();
        frame->ip -= offset;
        break;
      }

      case Op_code::call_: {
        int argCount = READ_BYTE();
        if (!call_value(peek(argCount), argCount)) {
          return INTERPRET_RUNTIME_ERROR;
        }
        frame = &vmp->frames.back();
        break;
      }

      case Op_code::invoke_: {
        ObjString* method = READ_STRING();
        int argCount = READ_BYTE();
        if (!invoke(method, argCount)) {
          return INTERPRET_RUNTIME_ERROR;
        }
        frame = &vmp->frames.back();
        break;
      }

      case Op_code::super_invoke_: {
        ObjString* method = READ_STRING();
        int argCount = READ_BYTE();
        ObjClass* superclass = boost::get<ObjClass*>((pop().variant));
        if (!invokeFromClass(superclass, method, argCount)) {
          return INTERPRET_RUNTIME_ERROR;
        }
        frame = &vmp->frames.back();
        break;
      }

      case Op_code::closure_: {
        ObjFunction* function = boost::get<ObjFunction*>((READ_CONSTANT().variant));
        ObjClosure* closure = deferred_heap_.make<ObjClosure>(function);
        vmp->stack.push_back(Value{closure});
        for (int i = 0; i < closure->function->upvalueCount; i++) {
          uint8_t isLocal = READ_BYTE();
          uint8_t index = READ_BYTE();
          if (isLocal) {
            closure->upvalues.push_back(captureUpvalue(&*(frame->slots + index)));
          } else {
            closure->upvalues.push_back(frame->closure->upvalues.at(index));
          }
        }
        break;
      }

      case Op_code::close_upvalue_:
        closeUpvalues(&*(vmp->stack.end() - 1));
        pop();
        break;

      case Op_code::return_: {
        Value result = pop();

        closeUpvalues(&*frame->slots);

        vmp->frames.pop_back();
        if (vmp->frames.size() == 0) {
          pop();
          return INTERPRET_OK;
        }

        vmp->stack.erase(frame->slots, vmp->stack.end());
        vmp->stack.push_back(result);

        frame = &vmp->frames.back();
        break;
      }

      case Op_code::class_:
        vmp->stack.push_back(Value{deferred_heap_.make<ObjClass>(READ_STRING())});
        break;

      case Op_code::inherit_: {
        Value superclass = peek(1);
        if (! boost::apply_visitor(Is_class_visitor{}, superclass.variant)) {
          runtimeError("Superclass must be a class.");
          return INTERPRET_RUNTIME_ERROR;
        }

        ObjClass* subclass = boost::get<ObjClass*>((peek(0).variant));
        for (const auto& pair : boost::get<ObjClass*>(superclass.variant)->methods) {
          subclass->methods[pair.first] = pair.second;
        }
        pop(); // Subclass.
        break;
      }

      case Op_code::method_:
        defineMethod(READ_STRING());
        break;
    }
  }

#undef READ_BYTE
#undef READ_SHORT
#undef READ_CONSTANT
#undef READ_STRING
#undef BINARY_OP
}
void hack(bool b) {
  // Hack to avoid unused function error. run() is not used in the
  // scanning chapter.
  run();
  if (b) hack(false);
}
InterpretResult interpret(VM& /*vmr*/, const char* source) {
  ObjFunction* function = compile(source);
  if (function == NULL) return INTERPRET_COMPILE_ERROR;

  vmp->stack.push_back(Value{function});
  ObjClosure* closure = deferred_heap_.make<ObjClosure>(function);
  pop();
  vmp->stack.push_back(Value{closure});
  call_value(Value{closure}, 0);

  return run();
}
