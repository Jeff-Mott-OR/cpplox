#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.hpp"
#include "compiler.hpp"
#include "object.hpp"
#include "scanner.hpp"

#ifdef DEBUG_PRINT_CODE
#include "debug.hpp"
#endif

typedef struct {
  Token current;
  Token previous;
  bool hadError;
  bool panicMode;
} Parser;

enum class Precedence {
  none_,
  assignment_,  // =
  or_,          // or
  and_,         // and
  equality_,    // == !=
  comparison_,  // < > <= >=
  term_,        // + -
  factor_,      // * /
  unary_,       // ! -
  call_,        // . ()
  primary_
};

typedef void (*ParseFn)(Parser& parser, Scanner&, bool canAssign);

typedef struct {
  ParseFn prefix;
  ParseFn infix;
  Precedence precedence;
} ParseRule;

typedef struct {
  Token name;
  int depth;
  bool isCaptured;
} Local;
typedef struct {
  uint8_t index;
  bool isLocal;
} Upvalue;
typedef enum {
  TYPE_FUNCTION,
  TYPE_INITIALIZER,
  TYPE_METHOD,
  TYPE_SCRIPT
} FunctionType;

struct Compiler;
Compiler* current = NULL;

struct Compiler {
  Compiler* enclosing;
  ObjFunction* function;
  FunctionType type;

  Local locals[UINT8_COUNT];
  int localCount;
  Upvalue upvalues[UINT8_COUNT];
  int scopeDepth;

  Compiler(Parser& parser, FunctionType type) {
    // static void initCompiler(Compiler* compiler, FunctionType type) {
      this->enclosing = current;
      this->function = NULL;
      this->type = type;
      this->localCount = 0;
      this->scopeDepth = 0;
      this->function = deferred_heap_.make<ObjFunction>();
      current = this;

      if (type != TYPE_SCRIPT) {
        current->function->name = interned_strings_.get(std::string{parser.previous.start, parser.previous.start + parser.previous.length});
      }

      Local* local = &current->locals[current->localCount++];
      local->depth = 0;
      local->isCaptured = false;
      if (type != TYPE_FUNCTION) {
        local->name.start = "this";
        local->name.length = 4;
      } else {
        local->name.start = "";
        local->name.length = 0;
      }
    // }
  }
};

typedef struct ClassCompiler {
  struct ClassCompiler* enclosing;
  Token name;
  bool hasSuperclass;
} ClassCompiler;

ClassCompiler* currentClass = NULL;

static Chunk* currentChunk() {
  return &current->function->chunk;
}

static void errorAt(Parser& parser, Token* token, const char* message) {
  if (parser.panicMode) return;
  parser.panicMode = true;

  fprintf(stderr, "[line %d] Error", token->line);

  if (token->type == Token_type::eof_) {
    fprintf(stderr, " at end");
  } else if (token->type == Token_type::error_) {
    // Nothing.
  } else {
    fprintf(stderr, " at '%.*s'", token->length, token->start);
  }

  fprintf(stderr, ": %s\n", message);
  parser.hadError = true;
}
static void error(Parser& parser, const char* message) {
  errorAt(parser, &parser.previous, message);
}
static void errorAtCurrent(Parser& parser, const char* message) {
  errorAt(parser, &parser.current, message);
}

static void advance(Parser& parser, Scanner& scanner) {
  parser.previous = parser.current;

  for (;;) {
    parser.current = scanToken(scanner);
    if (parser.current.type != Token_type::error_) break;

    errorAtCurrent(parser, parser.current.start);
  }
}
static void consume(Parser& parser, Scanner& scanner, Token_type type, const char* message) {
  if (parser.current.type == type) {
    advance(parser, scanner);
    return;
  }

  errorAtCurrent(parser, message);
}
static bool check(Parser& parser, Token_type type) {
  return parser.current.type == type;
}
static bool match(Parser& parser, Scanner& scanner, Token_type type) {
  if (!check(parser, type)) return false;
  advance(parser, scanner);
  return true;
}
static void emitByte(Parser& parser, uint8_t byte) {
    currentChunk()->code.push_back(byte);
    currentChunk()->lines.push_back(parser.previous.line);
}
static void emitByte(Parser& parser, Op_code byte) {
  emitByte(parser, static_cast<uint8_t>(byte));
}
template<typename T, typename U>
  static void emitBytes(Parser& parser, T byte1, U byte2) {
    emitByte(parser, byte1);
    emitByte(parser, byte2);
  }
static void emitLoop(Parser& parser, int loopStart) {
  emitByte(parser, Op_code::loop_);

  int offset = currentChunk()->code.size() - loopStart + 2;
  if (offset > UINT16_MAX) error(parser, "Loop body too large.");

  emitByte(parser, (offset >> 8) & 0xff);
  emitByte(parser, offset & 0xff);
}
static int emitJump(Parser& parser, uint8_t instruction) {
  emitByte(parser, instruction);
  emitByte(parser, 0xff);
  emitByte(parser, 0xff);
  return currentChunk()->code.size() - 2;
}
static int emitJump(Parser& parser, Op_code instruction) {
  return emitJump(parser, static_cast<uint8_t>(instruction));
}
static void emitReturn(Parser& parser) {
  if (current->type == TYPE_INITIALIZER) {
    emitBytes(parser, Op_code::get_local_, 0);
  } else {
    emitByte(parser, Op_code::nil_);
  }

  emitByte(parser, Op_code::return_);
}
static uint8_t makeConstant(Parser& parser, Value value) {
  currentChunk()->constants.push_back(value);
  int constant = currentChunk()->constants.size() - 1;
  if (constant > UINT8_MAX) {
    error(parser, "Too many constants in one chunk.");
    return 0;
  }

  return (uint8_t)constant;
}
static void emitConstant(Parser& parser, Value value) {
  emitBytes(parser, Op_code::constant_, makeConstant(parser, value));
}
static void patchJump(Parser& parser, int offset) {
  // -2 to adjust for the bytecode for the jump offset itself.
  int jump = currentChunk()->code.size() - offset - 2;

  if (jump > UINT16_MAX) {
    error(parser, "Too much code to jump over.");
  }

  currentChunk()->code.at(offset) = (jump >> 8) & 0xff;
  currentChunk()->code.at(offset + 1) = jump & 0xff;
}

static ObjFunction* endCompiler(Parser& parser) {
  emitReturn(parser);
  ObjFunction* function = current->function;

#ifdef DEBUG_PRINT_CODE
  if (!parser.hadError) {
    disassemble_chunk(*currentChunk(), function->name != NULL ? function->name->str.c_str() : "<script>");
  }
#endif

  current = current->enclosing;
  return function;
}
static void beginScope() {
  current->scopeDepth++;
}
static void endScope(Parser& parser) {
  current->scopeDepth--;

  while (current->localCount > 0 &&
         current->locals[current->localCount - 1].depth >
            current->scopeDepth) {
    if (current->locals[current->localCount - 1].isCaptured) {
      emitByte(parser, Op_code::close_upvalue_);
    } else {
      emitByte(parser, Op_code::pop_);
    }
    current->localCount--;
  }
}

static void expression(Parser& parser, Scanner&);
static void statement(Parser& parser, Scanner&);
static void declaration(Parser& parser, Scanner&);
static ParseRule* getRule(Token_type type);
static void parsePrecedence(Parser& parser, Scanner& scanner, Precedence precedence);

static uint8_t identifierConstant(Parser& parser, Token* name) {
  return makeConstant(parser, Value{interned_strings_.get(std::string{name->start, name->start + name->length})});
}
static bool identifiersEqual(Token* a, Token* b) {
  if (a->length != b->length) return false;
  return memcmp(a->start, b->start, a->length) == 0;
}
static int resolveLocal(Parser& parser, Compiler* compiler, Token* name) {
  for (int i = compiler->localCount - 1; i >= 0; i--) {
    Local* local = &compiler->locals[i];
    if (identifiersEqual(name, &local->name)) {
      if (local->depth == -1) {
        error(parser, "Cannot read local variable in its own initializer.");
      }
      return i;
    }
  }

  return -1;
}
static int addUpvalue(Parser& parser, Compiler* compiler, uint8_t index, bool isLocal) {
  int upvalueCount = compiler->function->upvalueCount;

  for (int i = 0; i < upvalueCount; i++) {
    Upvalue* upvalue = &compiler->upvalues[i];
    if (upvalue->index == index && upvalue->isLocal == isLocal) {
      return i;
    }
  }

  if (upvalueCount == UINT8_COUNT) {
    error(parser, "Too many closure variables in function.");
    return 0;
  }

  compiler->upvalues[upvalueCount].isLocal = isLocal;
  compiler->upvalues[upvalueCount].index = index;
  return compiler->function->upvalueCount++;
}
static int resolveUpvalue(Parser& parser, Compiler* compiler, Token* name) {
  if (compiler->enclosing == NULL) return -1;

  int local = resolveLocal(parser, compiler->enclosing, name);
  if (local != -1) {
    compiler->enclosing->locals[local].isCaptured = true;
    return addUpvalue(parser, compiler, (uint8_t)local, true);
  }

  int upvalue = resolveUpvalue(parser, compiler->enclosing, name);
  if (upvalue != -1) {
    return addUpvalue(parser, compiler, (uint8_t)upvalue, false);
  }

  return -1;
}
static void addLocal(Parser& parser, Token name) {
  if (current->localCount == UINT8_COUNT) {
    error(parser, "Too many local variables in function.");
    return;
  }

  Local* local = &current->locals[current->localCount++];
  local->name = name;
  local->depth = -1;
  local->isCaptured = false;
}
static void declareVariable(Parser& parser) {
  // Global variables are implicitly declared.
  if (current->scopeDepth == 0) return;

  Token* name = &parser.previous;
  for (int i = current->localCount - 1; i >= 0; i--) {
    Local* local = &current->locals[i];
    if (local->depth != -1 && local->depth < current->scopeDepth) {
      break; // [negative]
    }

    if (identifiersEqual(name, &local->name)) {
      error(parser, "Variable with this name already declared in this scope.");
    }
  }

  addLocal(parser, *name);
}
static uint8_t parseVariable(Parser& parser, Scanner& scanner, const char* errorMessage) {
  consume(parser, scanner, Token_type::identifier_, errorMessage);

  declareVariable(parser);
  if (current->scopeDepth > 0) return 0;

  return identifierConstant(parser, &parser.previous);
}
static void markInitialized() {
  if (current->scopeDepth == 0) return;
  current->locals[current->localCount - 1].depth =
      current->scopeDepth;
}
static void defineVariable(Parser& parser, uint8_t global) {
  if (current->scopeDepth > 0) {
    markInitialized();
    return;
  }

  emitBytes(parser, Op_code::define_global_, global);
}
static uint8_t argumentList(Parser& parser, Scanner& scanner) {
  uint8_t argCount = 0;
  if (!check(parser, Token_type::right_paren_)) {
    do {
      expression(parser, scanner);

      if (argCount == 255) {
        error(parser, "Cannot have more than 255 arguments.");
      }
      argCount++;
    } while (match(parser, scanner, Token_type::comma_));
  }

  consume(parser, scanner, Token_type::right_paren_, "Expect ')' after arguments.");
  return argCount;
}
static void and_(Parser& parser, Scanner& scanner, bool /*canAssign*/) {
  int endJump = emitJump(parser, Op_code::jump_if_false_);

  emitByte(parser, Op_code::pop_);
  parsePrecedence(parser, scanner, Precedence::and_);

  patchJump(parser, endJump);
}
static void binary(Parser& parser, Scanner& scanner, bool /*canAssign*/) {
  // Remember the operator.
  Token_type operatorType = parser.previous.type;

  // Compile the right operand.
  ParseRule* rule = getRule(operatorType);
  parsePrecedence(parser, scanner, static_cast<Precedence>(static_cast<int>(rule->precedence) + 1));

  // Emit the operator instruction.
  switch (operatorType) {
    case Token_type::bang_equal_:    emitBytes(parser, Op_code::equal_, Op_code::not_); break;
    case Token_type::equal_equal_:   emitByte(parser, Op_code::equal_); break;
    case Token_type::greater_:       emitByte(parser, Op_code::greater_); break;
    case Token_type::greater_equal_: emitBytes(parser, Op_code::less_, Op_code::not_); break;
    case Token_type::less_:          emitByte(parser, Op_code::less_); break;
    case Token_type::less_equal_:    emitBytes(parser, Op_code::greater_, Op_code::not_); break;
    case Token_type::plus_:          emitByte(parser, Op_code::add_); break;
    case Token_type::minus_:         emitByte(parser, Op_code::subtract_); break;
    case Token_type::star_:          emitByte(parser, Op_code::multiply_); break;
    case Token_type::slash_:         emitByte(parser, Op_code::divide_); break;
    default:
      return; // Unreachable.
  }
}
static void call(Parser& parser, Scanner& scanner, bool /*canAssign*/) {
  uint8_t argCount = argumentList(parser, scanner);
  emitBytes(parser, Op_code::call_, argCount);
}
static void dot(Parser& parser, Scanner& scanner, bool canAssign) {
  consume(parser, scanner, Token_type::identifier_, "Expect property name after '.'.");
  uint8_t name = identifierConstant(parser, &parser.previous);

  if (canAssign && match(parser, scanner, Token_type::equal_)) {
    expression(parser, scanner);
    emitBytes(parser, Op_code::set_property_, name);
  } else if (match(parser, scanner, Token_type::left_paren_)) {
    uint8_t argCount = argumentList(parser, scanner);
    emitBytes(parser, Op_code::invoke_, name);
    emitByte(parser, argCount);
  } else {
    emitBytes(parser, Op_code::get_property_, name);
  }
}
static void literal(Parser& parser, Scanner& /*scanner*/, bool /*canAssign*/) {
  switch (parser.previous.type) {
    case Token_type::false_: emitByte(parser, Op_code::false_); break;
    case Token_type::nil_: emitByte(parser, Op_code::nil_); break;
    case Token_type::true_: emitByte(parser, Op_code::true_); break;
    default:
      return; // Unreachable.
  }
}
static void grouping(Parser& parser, Scanner& scanner, bool /*canAssign*/) {
  expression(parser, scanner);
  consume(parser, scanner, Token_type::right_paren_, "Expect ')' after expression.");
}
static void number(Parser& parser, Scanner& /*scanner*/, bool /*canAssign*/) {
  double value = strtod(parser.previous.start, NULL);
  emitConstant(parser, Value{value});
}
static void or_(Parser& parser, Scanner& scanner, bool /*canAssign*/) {
  int elseJump = emitJump(parser, Op_code::jump_if_false_);
  int endJump = emitJump(parser, Op_code::jump_);

  patchJump(parser, elseJump);
  emitByte(parser, Op_code::pop_);

  parsePrecedence(parser, scanner, Precedence::or_);
  patchJump(parser, endJump);
}
static void string(Parser& parser, Scanner& /*scanner*/, bool /*canAssign*/) {
  emitConstant(parser, Value{interned_strings_.get(std::string{parser.previous.start + 1, parser.previous.start + parser.previous.length - 1})});
}
static void namedVariable(Parser& parser, Scanner& scanner, Token name, bool canAssign) {
  Op_code getOp, setOp;
  int arg = resolveLocal(parser, current, &name);
  if (arg != -1) {
    getOp = Op_code::get_local_;
    setOp = Op_code::set_local_;
  } else if ((arg = resolveUpvalue(parser, current, &name)) != -1) {
    getOp = Op_code::get_upvalue_;
    setOp = Op_code::set_upvalue_;
  } else {
    arg = identifierConstant(parser, &name);
    getOp = Op_code::get_global_;
    setOp = Op_code::set_global_;
  }

  if (canAssign && match(parser, scanner, Token_type::equal_)) {
    expression(parser, scanner);
    emitBytes(parser, setOp, arg);
  } else {
    emitBytes(parser, getOp, arg);
  }
}
static void variable(Parser& parser, Scanner& scanner, bool canAssign) {
  namedVariable(parser, scanner, parser.previous, canAssign);
}
static Token syntheticToken(const char* text) {
  Token token;
  token.start = text;
  token.length = (int)strlen(text);
  return token;
}
static void super_(Parser& parser, Scanner& scanner, bool /*canAssign*/) {
  if (currentClass == NULL) {
    error(parser, "Cannot use 'super' outside of a class.");
  } else if (!currentClass->hasSuperclass) {
    error(parser, "Cannot use 'super' in a class with no superclass.");
  }

  consume(parser, scanner, Token_type::dot_, "Expect '.' after 'super'.");
  consume(parser, scanner, Token_type::identifier_, "Expect superclass method name.");
  uint8_t name = identifierConstant(parser, &parser.previous);

  namedVariable(parser, scanner, syntheticToken("this"), false);
  if (match(parser, scanner, Token_type::left_paren_)) {
    uint8_t argCount = argumentList(parser, scanner);
    namedVariable(parser, scanner, syntheticToken("super"), false);
    emitBytes(parser, Op_code::super_invoke_, name);
    emitByte(parser, argCount);
  } else {
    namedVariable(parser, scanner, syntheticToken("super"), false);
    emitBytes(parser, Op_code::get_super_, name);
  }
}
static void this_(Parser& parser, Scanner& scanner, bool /*canAssign*/) {
  if (currentClass == NULL) {
    error(parser, "Cannot use 'this' outside of a class.");
    return;
  }
  variable(parser, scanner, false);
} // [this]
static void unary(Parser& parser, Scanner& scanner, bool /*canAssign*/) {
  Token_type operatorType = parser.previous.type;

  // Compile the operand.
  parsePrecedence(parser, scanner, Precedence::unary_);

  // Emit the operator instruction.
  switch (operatorType) {
    case Token_type::bang_: emitByte(parser, Op_code::not_); break;
    case Token_type::minus_: emitByte(parser, Op_code::negate_); break;
    default:
      return; // Unreachable.
  }
}
ParseRule rules[] = {
  [static_cast<int>(Token_type::left_paren_)]    = { grouping, call,   Precedence::call_ },
  [static_cast<int>(Token_type::right_paren_)]   = { NULL,     NULL,   Precedence::none_ },
  [static_cast<int>(Token_type::left_brace_)]    = { NULL,     NULL,   Precedence::none_ },
  [static_cast<int>(Token_type::right_brace_)]   = { NULL,     NULL,   Precedence::none_ },
  [static_cast<int>(Token_type::comma_)]         = { NULL,     NULL,   Precedence::none_ },
  [static_cast<int>(Token_type::dot_)]           = { NULL,     dot,    Precedence::call_ },
  [static_cast<int>(Token_type::minus_)]         = { unary,    binary, Precedence::term_ },
  [static_cast<int>(Token_type::plus_)]          = { NULL,     binary, Precedence::term_ },
  [static_cast<int>(Token_type::semicolon_)]     = { NULL,     NULL,   Precedence::none_ },
  [static_cast<int>(Token_type::slash_)]         = { NULL,     binary, Precedence::factor_ },
  [static_cast<int>(Token_type::star_)]          = { NULL,     binary, Precedence::factor_ },
  [static_cast<int>(Token_type::bang_)]          = { unary,    NULL,   Precedence::none_ },
  [static_cast<int>(Token_type::bang_equal_)]    = { NULL,     binary, Precedence::equality_ },
  [static_cast<int>(Token_type::equal_)]         = { NULL,     NULL,   Precedence::none_ },
  [static_cast<int>(Token_type::equal_equal_)]   = { NULL,     binary, Precedence::equality_ },
  [static_cast<int>(Token_type::greater_)]       = { NULL,     binary, Precedence::comparison_ },
  [static_cast<int>(Token_type::greater_equal_)] = { NULL,     binary, Precedence::comparison_ },
  [static_cast<int>(Token_type::less_)]          = { NULL,     binary, Precedence::comparison_ },
  [static_cast<int>(Token_type::less_equal_)]    = { NULL,     binary, Precedence::comparison_ },
  [static_cast<int>(Token_type::identifier_)]    = { variable, NULL,   Precedence::none_ },
  [static_cast<int>(Token_type::string_)]        = { string,   NULL,   Precedence::none_ },
  [static_cast<int>(Token_type::number_)]        = { number,    NULL,   Precedence::none_ },
  [static_cast<int>(Token_type::and_)]           = { NULL,     and_,   Precedence::and_ },
  [static_cast<int>(Token_type::class_)]         = { NULL,     NULL,   Precedence::none_ },
  [static_cast<int>(Token_type::else_)]          = { NULL,     NULL,   Precedence::none_ },
  [static_cast<int>(Token_type::false_)]         = { literal,  NULL,   Precedence::none_ },
  [static_cast<int>(Token_type::for_)]           = { NULL,     NULL,   Precedence::none_ },
  [static_cast<int>(Token_type::fun_)]           = { NULL,     NULL,   Precedence::none_ },
  [static_cast<int>(Token_type::if_)]            = { NULL,     NULL,   Precedence::none_ },
  [static_cast<int>(Token_type::nil_)]           = { literal,  NULL,   Precedence::none_ },
  [static_cast<int>(Token_type::or_)]            = { NULL,     or_,    Precedence::or_ },
  [static_cast<int>(Token_type::print_)]         = { NULL,     NULL,   Precedence::none_ },
  [static_cast<int>(Token_type::return_)]        = { NULL,     NULL,   Precedence::none_ },
  [static_cast<int>(Token_type::super_)]         = { super_,   NULL,   Precedence::none_ },
  [static_cast<int>(Token_type::this_)]          = { this_,    NULL,   Precedence::none_ },
  [static_cast<int>(Token_type::true_)]          = { literal,  NULL,   Precedence::none_ },
  [static_cast<int>(Token_type::var_)]           = { NULL,     NULL,   Precedence::none_ },
  [static_cast<int>(Token_type::while_)]         = { NULL,     NULL,   Precedence::none_ },
  [static_cast<int>(Token_type::error_)]         = { NULL,     NULL,   Precedence::none_ },
  [static_cast<int>(Token_type::eof_)]           = { NULL,     NULL,   Precedence::none_ },
};
static void parsePrecedence(Parser& parser, Scanner& scanner, Precedence precedence) {
  advance(parser, scanner);
  ParseFn prefixRule = getRule(parser.previous.type)->prefix;
  if (prefixRule == NULL) {
    error(parser, "Expect expression.");
    return;
  }

  bool canAssign = precedence <= Precedence::assignment_;
  prefixRule(parser, scanner, canAssign);

  while (precedence <= getRule(parser.current.type)->precedence) {
    advance(parser, scanner);
    ParseFn infixRule = getRule(parser.previous.type)->infix;
    infixRule(parser, scanner, canAssign);
  }

  if (canAssign && match(parser, scanner, Token_type::equal_)) {
    error(parser, "Invalid assignment target.");
  }
}
static ParseRule* getRule(Token_type type) {
  return &rules[static_cast<int>(type)];
}
static void expression(Parser& parser, Scanner& scanner) {
  parsePrecedence(parser, scanner, Precedence::assignment_);
}
static void block(Parser& parser, Scanner& scanner) {
  while (!check(parser, Token_type::right_brace_) && !check(parser, Token_type::eof_)) {
    declaration(parser, scanner);
  }

  consume(parser, scanner, Token_type::right_brace_, "Expect '}' after block.");
}
static void function(Parser& parser, Scanner& scanner, FunctionType type) {
  Compiler compiler {parser, type};
  beginScope(); // [no-end-scope]

  // Compile the parameter list.
  consume(parser, scanner, Token_type::left_paren_, "Expect '(' after function name.");
  if (!check(parser, Token_type::right_paren_)) {
    do {
      current->function->arity++;
      if (current->function->arity > 255) {
        errorAtCurrent(parser, "Cannot have more than 255 parameters.");
      }

      uint8_t paramConstant = parseVariable(parser, scanner, "Expect parameter name.");
      defineVariable(parser, paramConstant);
    } while (match(parser, scanner, Token_type::comma_));
  }
  consume(parser, scanner, Token_type::right_paren_, "Expect ')' after parameters.");

  // The body.
  consume(parser, scanner, Token_type::left_brace_, "Expect '{' before function body.");
  block(parser, scanner);

  // Create the function object.
  ObjFunction* function = endCompiler(parser);
  emitBytes(parser, Op_code::closure_, makeConstant(parser, Value{function}));

  for (int i = 0; i < function->upvalueCount; i++) {
    emitByte(parser, compiler.upvalues[i].isLocal ? 1 : 0);
    emitByte(parser, compiler.upvalues[i].index);
  }
}
static void method(Parser& parser, Scanner& scanner) {
  consume(parser, scanner, Token_type::identifier_, "Expect method name.");
  uint8_t constant = identifierConstant(parser, &parser.previous);

  FunctionType type = TYPE_METHOD;
  if (parser.previous.length == 4 &&
      memcmp(parser.previous.start, "init", 4) == 0) {
    type = TYPE_INITIALIZER;
  }

  function(parser, scanner, type);
  emitBytes(parser, Op_code::method_, constant);
}
static void classDeclaration(Parser& parser, Scanner& scanner) {
  consume(parser, scanner, Token_type::identifier_, "Expect class name.");
  Token className = parser.previous;
  uint8_t nameConstant = identifierConstant(parser, &parser.previous);
  declareVariable(parser);

  emitBytes(parser, Op_code::class_, nameConstant);
  defineVariable(parser, nameConstant);

  ClassCompiler classCompiler;
  classCompiler.name = parser.previous;
  classCompiler.hasSuperclass = false;
  classCompiler.enclosing = currentClass;
  currentClass = &classCompiler;

  if (match(parser, scanner, Token_type::less_)) {
    consume(parser, scanner, Token_type::identifier_, "Expect superclass name.");
    variable(parser, scanner, false);

    if (identifiersEqual(&className, &parser.previous)) {
      error(parser, "A class cannot inherit from itself.");
    }

    beginScope();
    addLocal(parser, syntheticToken("super"));
    defineVariable(parser, 0);

    namedVariable(parser, scanner, className, false);
    emitByte(parser, Op_code::inherit_);
    classCompiler.hasSuperclass = true;
  }

  namedVariable(parser, scanner, className, false);
  consume(parser, scanner, Token_type::left_brace_, "Expect '{' before class body.");
  while (!check(parser, Token_type::right_brace_) && !check(parser, Token_type::eof_)) {
    method(parser, scanner);
  }
  consume(parser, scanner, Token_type::right_brace_, "Expect '}' after class body.");
  emitByte(parser, Op_code::pop_);

  if (classCompiler.hasSuperclass) {
    endScope(parser);
  }

  currentClass = currentClass->enclosing;
}
static void funDeclaration(Parser& parser, Scanner& scanner) {
  uint8_t global = parseVariable(parser, scanner, "Expect function name.");
  markInitialized();
  function(parser, scanner, TYPE_FUNCTION);
  defineVariable(parser, global);
}
static void varDeclaration(Parser& parser, Scanner& scanner) {
  uint8_t global = parseVariable(parser, scanner, "Expect variable name.");

  if (match(parser, scanner, Token_type::equal_)) {
    expression(parser, scanner);
  } else {
    emitByte(parser, Op_code::nil_);
  }
  consume(parser, scanner, Token_type::semicolon_, "Expect ';' after variable declaration.");

  defineVariable(parser, global);
}
static void expressionStatement(Parser& parser, Scanner& scanner) {
  expression(parser, scanner);
  consume(parser, scanner, Token_type::semicolon_, "Expect ';' after expression.");
  emitByte(parser, Op_code::pop_);
}
static void forStatement(Parser& parser, Scanner& scanner) {
  beginScope();

  consume(parser, scanner, Token_type::left_paren_, "Expect '(' after 'for'.");
  if (match(parser, scanner, Token_type::semicolon_)) {
    // No initializer.
  } else if (match(parser, scanner, Token_type::var_)) {
    varDeclaration(parser, scanner);
  } else {
    expressionStatement(parser, scanner);
  }

  int loopStart = currentChunk()->code.size();

  int exitJump = -1;
  if (!match(parser, scanner, Token_type::semicolon_)) {
    expression(parser, scanner);
    consume(parser, scanner, Token_type::semicolon_, "Expect ';' after loop condition.");

    // Jump out of the loop if the condition is false.
    exitJump = emitJump(parser, Op_code::jump_if_false_);
    emitByte(parser, Op_code::pop_); // Condition.
  }

  if (!match(parser, scanner, Token_type::right_paren_)) {
    int bodyJump = emitJump(parser, Op_code::jump_);

    int incrementStart = currentChunk()->code.size();
    expression(parser, scanner);
    emitByte(parser, Op_code::pop_);
    consume(parser, scanner, Token_type::right_paren_, "Expect ')' after for clauses.");

    emitLoop(parser, loopStart);
    loopStart = incrementStart;
    patchJump(parser, bodyJump);
  }

  statement(parser, scanner);

  emitLoop(parser, loopStart);

  if (exitJump != -1) {
    patchJump(parser, exitJump);
    emitByte(parser, Op_code::pop_); // Condition.
  }

  endScope(parser);
}
static void ifStatement(Parser& parser, Scanner& scanner) {
  consume(parser, scanner, Token_type::left_paren_, "Expect '(' after 'if'.");
  expression(parser, scanner);
  consume(parser, scanner, Token_type::right_paren_, "Expect ')' after condition."); // [paren]

  int thenJump = emitJump(parser, Op_code::jump_if_false_);
  emitByte(parser, Op_code::pop_);
  statement(parser, scanner);

  int elseJump = emitJump(parser, Op_code::jump_);

  patchJump(parser, thenJump);
  emitByte(parser, Op_code::pop_);

  if (match(parser, scanner, Token_type::else_)) statement(parser, scanner);
  patchJump(parser, elseJump);
}
static void printStatement(Parser& parser, Scanner& scanner) {
  expression(parser, scanner);
  consume(parser, scanner, Token_type::semicolon_, "Expect ';' after value.");
  emitByte(parser, Op_code::print_);
}
static void returnStatement(Parser& parser, Scanner& scanner) {
  if (current->type == TYPE_SCRIPT) {
    error(parser, "Cannot return from top-level code.");
  }

  if (match(parser, scanner, Token_type::semicolon_)) {
    emitReturn(parser);
  } else {
    if (current->type == TYPE_INITIALIZER) {
      error(parser, "Cannot return a value from an initializer.");
    }

    expression(parser, scanner);
    consume(parser, scanner, Token_type::semicolon_, "Expect ';' after return value.");
    emitByte(parser, Op_code::return_);
  }
}
static void whileStatement(Parser& parser, Scanner& scanner) {
  int loopStart = currentChunk()->code.size();

  consume(parser, scanner, Token_type::left_paren_, "Expect '(' after 'while'.");
  expression(parser, scanner);
  consume(parser, scanner, Token_type::right_paren_, "Expect ')' after condition.");

  int exitJump = emitJump(parser, Op_code::jump_if_false_);

  emitByte(parser, Op_code::pop_);
  statement(parser, scanner);

  emitLoop(parser, loopStart);

  patchJump(parser, exitJump);
  emitByte(parser, Op_code::pop_);
}
static void synchronize(Parser& parser, Scanner& scanner) {
  parser.panicMode = false;

  while (parser.current.type != Token_type::eof_) {
    if (parser.previous.type == Token_type::semicolon_) return;

    switch (parser.current.type) {
      case Token_type::class_:
      case Token_type::fun_:
      case Token_type::var_:
      case Token_type::for_:
      case Token_type::if_:
      case Token_type::while_:
      case Token_type::print_:
      case Token_type::return_:
        return;

      default:
        // Do nothing.
        ;
    }

    advance(parser, scanner);
  }
}
static void declaration(Parser& parser, Scanner& scanner) {
  if (match(parser, scanner, Token_type::class_)) {
    classDeclaration(parser, scanner);
  } else if (match(parser, scanner, Token_type::fun_)) {
    funDeclaration(parser, scanner);
  } else if (match(parser, scanner, Token_type::var_)) {
    varDeclaration(parser, scanner);
  } else {
    statement(parser, scanner);
  }

  if (parser.panicMode) synchronize(parser, scanner);
}
static void statement(Parser& parser, Scanner& scanner) {
  if (match(parser, scanner, Token_type::print_)) {
    printStatement(parser, scanner);
  } else if (match(parser, scanner, Token_type::for_)) {
    forStatement(parser, scanner);
  } else if (match(parser, scanner, Token_type::if_)) {
    ifStatement(parser, scanner);
  } else if (match(parser, scanner, Token_type::return_)) {
    returnStatement(parser, scanner);
  } else if (match(parser, scanner, Token_type::while_)) {
    whileStatement(parser, scanner);
  } else if (match(parser, scanner, Token_type::left_brace_)) {
    beginScope();
    block(parser, scanner);
    endScope(parser);
  } else {
    expressionStatement(parser, scanner);
  }
}

ObjFunction* compile(const char* source) {
  Scanner scanner {source};
  Parser parser;
  Compiler compiler {parser, TYPE_SCRIPT};

  parser.hadError = false;
  parser.panicMode = false;

  advance(parser, scanner);

  while (!match(parser, scanner, Token_type::eof_)) {
    declaration(parser, scanner);
  }

  ObjFunction* function = endCompiler(parser);
  return parser.hadError ? NULL : function;
}
void markCompilerRoots() {
  Compiler* compiler = current;
  while (compiler != NULL) {
    deferred_heap_.mark(*compiler->function);
    compiler = compiler->enclosing;
  }
}
