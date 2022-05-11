#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <boost/lexical_cast.hpp>
#include <gsl/string_span>

#include "common.hpp"
#include "compiler.hpp"
#include "object.hpp"
#include "scanner.hpp"

#ifdef DEBUG_PRINT_CODE
#include "debug.hpp"
#endif

struct Parser {
  Token current;
  bool hadError = false;
  bool panicMode = false;
};

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

typedef void (*ParseFn)(const Token&, Parser&, Scanner&, bool canAssign);

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

enum class Function_type {
  function_,
  initializer_,
  method_,
  script_
};

struct Function_compiler;
Function_compiler* global_current = NULL;

struct Function_compiler {
  Function_compiler* enclosing;
  ObjFunction* function;
  Function_type type;
  Deferred_heap& deferred_heap_;
  Interned_strings& interned_strings_;

  std::vector<Local> locals;
  Upvalue upvalues[UINT8_COUNT];
  int scopeDepth;

  Function_compiler(Function_type type, const Token& token, Parser& /*parser*/, Deferred_heap& deferred_heap, Interned_strings& interned_strings) :
    deferred_heap_ {deferred_heap}, interned_strings_ {interned_strings}
  {
    // static void initCompiler(Function_compiler* compiler, Function_type type) {
      this->enclosing = global_current;
      this->function = deferred_heap_.make<ObjFunction>(type != Function_type::script_ ? interned_strings_.get(std::string{token.lexeme_begin, token.lexeme_end}) : nullptr);
      this->type = type;
      this->scopeDepth = 0;
      global_current = this;

      Local local;
      local.depth = 0;
      local.isCaptured = false;
      if (type != Function_type::function_) {
        local.name.lexeme_begin = "this";
        local.name.lexeme_end = local.name.lexeme_begin + 4;
      } else {
        local.name.lexeme_begin = "";
        local.name.lexeme_end = local.name.lexeme_begin;
      }
      locals.push_back(local);
    // }
  }
};

typedef struct ClassCompiler {
  struct ClassCompiler* enclosing;
  Token name;
  bool hasSuperclass;
} ClassCompiler;

ClassCompiler* currentClass = NULL;

static Token consume(Parser& parser, Scanner& scanner, Token_type type, const char* message) {
  if (parser.current.type == type) {
    auto token = parser.current;
    parser.current = scanToken(scanner);
    return token;
  }

  throw std::runtime_error{
      "[Line " + std::to_string(parser.current.line) +
      "] Error at '" + std::string{parser.current.lexeme_begin, parser.current.lexeme_end} +
      "': " + message + "\n"
  };
}
static bool advance_if_match(Parser& parser, Scanner& scanner, Token_type type) {
  if (parser.current.type != type) return false;
  parser.current = scanToken(scanner);
  return true;
}
static void emitByte(const Token& where, Parser& /*parser*/, uint8_t byte) {
    global_current->function->chunk.code.push_back(byte);
    global_current->function->chunk.lines.push_back(where.line);
}
static void emitByte(const Token& where, Parser& parser, Op_code byte) {
  emitByte(where, parser, static_cast<uint8_t>(byte));
}
template<typename T, typename U>
  static void emitBytes(const Token& where, Parser& parser, T byte1, U byte2) {
    emitByte(where, parser, byte1);
    emitByte(where, parser, byte2);
  }
static void emitLoop(const Token& where, Parser& parser, int loopStart) {
  emitByte(where, parser, Op_code::loop_);

  int offset = global_current->function->chunk.code.size() - loopStart + 2;
  if (offset > UINT16_MAX) {
    throw std::runtime_error{
        "[Line " + std::to_string(where.line) +
        "] Error at '" + std::string{where.lexeme_begin, where.lexeme_end} +
        "': Loop body too large.\n"
    };
  }

  emitByte(where, parser, (offset >> 8) & 0xff);
  emitByte(where, parser, offset & 0xff);
}
static int emitJump(const Token& where, Parser& parser, uint8_t instruction) {
  emitByte(where, parser, instruction);
  emitByte(where, parser, 0xff);
  emitByte(where, parser, 0xff);
  return global_current->function->chunk.code.size() - 2;
}
static int emitJump(const Token& where, Parser& parser, Op_code instruction) {
  return emitJump(where, parser, static_cast<uint8_t>(instruction));
}
static void emitReturn(const Token& where, Parser& parser) {
  if (global_current->type == Function_type::initializer_) {
    emitBytes(where, parser, Op_code::get_local_, 0);
  } else {
    emitByte(where, parser, Op_code::nil_);
  }

  emitByte(where, parser, Op_code::return_);
}
static uint8_t makeConstant(const Token& where, Parser& parser, Value value) {
  global_current->function->chunk.constants.push_back(value);
  int constant = global_current->function->chunk.constants.size() - 1;
  if (constant > UINT8_MAX) {
    throw std::runtime_error{
        "[Line " + std::to_string(where.line) +
        "] Error at '" + std::string{where.lexeme_begin, where.lexeme_end} +
        "': Too many constants in one chunk.\n"
    };
  }

  return (uint8_t)constant;
}
static void emitConstant(const Token& where, Parser& parser, Value value) {
  emitBytes(where, parser, Op_code::constant_, makeConstant(where, parser, value));
}
static void patchJump(const Token& token, Parser& parser, int offset) {
  // -2 to adjust for the bytecode for the jump offset itself.
  int jump = global_current->function->chunk.code.size() - offset - 2;

  if (jump > UINT16_MAX) {
    throw std::runtime_error{
        "[Line " + std::to_string(token.line) +
        "] Error at '" + std::string{token.lexeme_begin, token.lexeme_end} +
        "': Too much code to jump over.\n"
    };
  }

  global_current->function->chunk.code.at(offset) = (jump >> 8) & 0xff;
  global_current->function->chunk.code.at(offset + 1) = jump & 0xff;
}

static ObjFunction* endCompiler(const Token& where, Parser& parser) {
  emitReturn(where, parser);
  ObjFunction* function = global_current->function;

  global_current = global_current->enclosing;
  return function;
}
static void beginScope() {
  global_current->scopeDepth++;
}
static void endScope(const Token& where, Parser& parser) {
  global_current->scopeDepth--;

  while (!global_current->locals.empty() && global_current->locals.back().depth > global_current->scopeDepth) {
    if (global_current->locals.back().isCaptured) {
      emitByte(where, parser, Op_code::close_upvalue_);
    } else {
      emitByte(where, parser, Op_code::pop_);
    }
    global_current->locals.pop_back();
  }
}

static void expression(Parser& parser, Scanner&);
static void statement(Parser& parser, Scanner&);
static void consume_declaration(Parser& parser, Scanner&);
static ParseRule* getRule(Token_type type);
static void parsePrecedence(Parser& parser, Scanner& scanner, Precedence precedence);

static uint8_t identifierConstant(Parser& parser, const Token& name) {
  Value constant_value {global_current->interned_strings_.get(std::string{name.lexeme_begin, name.lexeme_end})};
  return makeConstant(name, parser, constant_value);
}
static bool identifiersEqual(const Token& a, const Token& b) {
  return gsl::cstring_span<>{a.lexeme_begin, a.lexeme_end} == gsl::cstring_span<>{b.lexeme_begin, b.lexeme_end};
}
static int resolveLocal(Parser& parser, Function_compiler* compiler, const Token& name) {
  for (auto local_iter = compiler->locals.crbegin(); local_iter != compiler->locals.crend(); ++local_iter) {
    if (identifiersEqual(name, local_iter->name)) {
      if (local_iter->depth == -1) {
        throw std::runtime_error{
            "[Line " + std::to_string(name.line) +
            "] Error at '" + std::string{name.lexeme_begin, name.lexeme_end} +
            "': Cannot read local variable in its own initializer.\n"
        };
      }
      return std::distance(compiler->locals.cbegin(), local_iter.base()) - 1;
    }
  }

  return -1;
}
static int addUpvalue(const Token& token, Parser& parser, Function_compiler* compiler, uint8_t index, bool isLocal) {
  int upvalueCount = compiler->function->upvalueCount;

  for (int i = 0; i < upvalueCount; i++) {
    Upvalue* upvalue = &compiler->upvalues[i];
    if (upvalue->index == index && upvalue->isLocal == isLocal) {
      return i;
    }
  }

  if (upvalueCount == UINT8_COUNT) {
    throw std::runtime_error{
        "[Line " + std::to_string(token.line) +
        "] Error at '" + std::string{token.lexeme_begin, token.lexeme_end} +
        "': Too many closure variables in function.\n"
    };
  }

  compiler->upvalues[upvalueCount].isLocal = isLocal;
  compiler->upvalues[upvalueCount].index = index;
  return compiler->function->upvalueCount++;
}
static int resolveUpvalue(Parser& parser, Function_compiler* compiler, const Token& name) {
  if (compiler->enclosing == NULL) return -1;

  int local = resolveLocal(parser, compiler->enclosing, name);
  if (local != -1) {
    compiler->enclosing->locals.at(local).isCaptured = true;
    return addUpvalue(name, parser, compiler, (uint8_t)local, true);
  }

  int upvalue = resolveUpvalue(parser, compiler->enclosing, name);
  if (upvalue != -1) {
    return addUpvalue(name, parser, compiler, (uint8_t)upvalue, false);
  }

  return -1;
}
static void addLocal(Parser& parser, Token name) {
  if (global_current->locals.size() == UINT8_COUNT) {
    throw std::runtime_error{
        "[Line " + std::to_string(name.line) +
        "] Error at '" + std::string{name.lexeme_begin, name.lexeme_end} +
        "': Too many local variables in function.\n"
    };
  }

  Local local;
  local.name = name;
  local.depth = -1;
  local.isCaptured = false;
  global_current->locals.push_back(local);
}
static void declareVariable(Token name, Parser& parser) {
  // Global variables are implicitly declared.
  if (global_current->scopeDepth == 0) return;

  for (auto local_iter = global_current->locals.crbegin(); local_iter != global_current->locals.crend(); ++local_iter) {
    if (local_iter->depth != -1 && local_iter->depth < global_current->scopeDepth) {
      break; // [negative]
    }

    if (identifiersEqual(name, local_iter->name)) {
      throw std::runtime_error{
          "[Line " + std::to_string(name.line) +
          "] Error at '" + std::string{name.lexeme_begin, name.lexeme_end} +
          "': Variable with this name already declared in this scope.\n"
      };
    }
  }

  addLocal(parser, name);
}
static uint8_t parseVariable(Parser& parser, Scanner& scanner, const char* errorMessage) {
  auto identifier_token = consume(parser, scanner, Token_type::identifier_, errorMessage);

  declareVariable(identifier_token, parser);
  if (global_current->scopeDepth > 0) return 0;

  return identifierConstant(parser, identifier_token);
}
static void markInitialized() {
  if (global_current->scopeDepth == 0) return;
  global_current->locals.back().depth = global_current->scopeDepth;
}
static void defineVariable(const Token& where, Parser& parser, uint8_t global) {
  if (global_current->scopeDepth > 0) {
    markInitialized();
    return;
  }

  emitBytes(where, parser, Op_code::define_global_, global);
}
static uint8_t argumentList(Parser& parser, Scanner& scanner) {
  uint8_t argCount = 0;
  if (parser.current.type != Token_type::right_paren_) {
    do {
      auto current_token = parser.current;
      expression(parser, scanner);

      if (argCount == 255) {
        throw std::runtime_error{
            "[Line " + std::to_string(current_token.line) +
            "] Error at '" + std::string{current_token.lexeme_begin, current_token.lexeme_end} +
            "': Cannot have more than 255 arguments.\n"
        };
      }
      argCount++;
    } while (advance_if_match(parser, scanner, Token_type::comma_));
  }

  consume(parser, scanner, Token_type::right_paren_, "Expected ')' after arguments.");
  return argCount;
}
static void and_(const Token& where, Parser& parser, Scanner& scanner, bool /*canAssign*/) {
  int endJump = emitJump(where, parser, Op_code::jump_if_false_);

  emitByte(where, parser, Op_code::pop_);
  parsePrecedence(parser, scanner, Precedence::and_);

  patchJump(where, parser, endJump);
}
static void binary(const Token& token, Parser& parser, Scanner& scanner, bool /*canAssign*/) {
  // Remember the operator.
  Token_type operatorType = token.type;

  // Compile the right operand.
  ParseRule* rule = getRule(operatorType);
  parsePrecedence(parser, scanner, static_cast<Precedence>(static_cast<int>(rule->precedence) + 1));

  // Emit the operator instruction.
  switch (operatorType) {
    case Token_type::bang_equal_:    emitBytes(token, parser, Op_code::equal_, Op_code::not_); break;
    case Token_type::equal_equal_:   emitByte(token, parser, Op_code::equal_); break;
    case Token_type::greater_:       emitByte(token, parser, Op_code::greater_); break;
    case Token_type::greater_equal_: emitBytes(token, parser, Op_code::less_, Op_code::not_); break;
    case Token_type::less_:          emitByte(token, parser, Op_code::less_); break;
    case Token_type::less_equal_:    emitBytes(token, parser, Op_code::greater_, Op_code::not_); break;
    case Token_type::plus_:          emitByte(token, parser, Op_code::add_); break;
    case Token_type::minus_:         emitByte(token, parser, Op_code::subtract_); break;
    case Token_type::star_:          emitByte(token, parser, Op_code::multiply_); break;
    case Token_type::slash_:         emitByte(token, parser, Op_code::divide_); break;
    default:
      return; // Unreachable.
  }
}
static void call(const Token& token, Parser& parser, Scanner& scanner, bool /*canAssign*/) {
  uint8_t argCount = argumentList(parser, scanner);
  emitBytes(token, parser, Op_code::call_, argCount);
}
static void dot(const Token& token, Parser& parser, Scanner& scanner, bool canAssign) {
  auto parser_previous_token = consume(parser, scanner, Token_type::identifier_, "Expected property name after '.'.");
  uint8_t name = identifierConstant(parser, parser_previous_token);

  if (canAssign && advance_if_match(parser, scanner, Token_type::equal_)) {
    expression(parser, scanner);
    emitBytes(token, parser, Op_code::set_property_, name);
  } else if (advance_if_match(parser, scanner, Token_type::left_paren_)) {
    uint8_t argCount = argumentList(parser, scanner);
    emitBytes(token, parser, Op_code::invoke_, name);
    emitByte(token, parser, argCount);
  } else {
    emitBytes(token, parser, Op_code::get_property_, name);
  }
}
static void literal(const Token& token, Parser& parser, Scanner& /*scanner*/, bool /*canAssign*/) {
  switch (token.type) {
    case Token_type::false_: emitByte(token, parser, Op_code::false_); break;
    case Token_type::nil_: emitByte(token, parser, Op_code::nil_); break;
    case Token_type::true_: emitByte(token, parser, Op_code::true_); break;
    default:
      return; // Unreachable.
  }
}
static void grouping(const Token& /*token*/, Parser& parser, Scanner& scanner, bool /*canAssign*/) {
  expression(parser, scanner);
  consume(parser, scanner, Token_type::right_paren_, "Expected ')' after expression.");
}
static void number(const Token& token, Parser& parser, Scanner& /*scanner*/, bool /*canAssign*/) {
  double value = boost::lexical_cast<double>(std::string{token.lexeme_begin, token.lexeme_end});
  emitConstant(token, parser, Value{value});
}
static void or_(const Token& token, Parser& parser, Scanner& scanner, bool /*canAssign*/) {
  int elseJump = emitJump(token, parser, Op_code::jump_if_false_);
  int endJump = emitJump(token, parser, Op_code::jump_);

  patchJump(token, parser, elseJump);
  emitByte(token, parser, Op_code::pop_);

  parsePrecedence(parser, scanner, Precedence::or_);
  patchJump(token, parser, endJump);
}
static void string(const Token& token, Parser& parser, Scanner& /*scanner*/, bool /*canAssign*/) {
  emitConstant(token, parser, Value{global_current->interned_strings_.get(std::string{token.lexeme_begin + 1, token.lexeme_end - 1})});
}
static void namedVariable(Parser& parser, Scanner& scanner, Token name, bool canAssign) {
  Op_code getOp, setOp;
  int arg = resolveLocal(parser, global_current, name);
  if (arg != -1) {
    getOp = Op_code::get_local_;
    setOp = Op_code::set_local_;
  } else if ((arg = resolveUpvalue(parser, global_current, name)) != -1) {
    getOp = Op_code::get_upvalue_;
    setOp = Op_code::set_upvalue_;
  } else {
    arg = identifierConstant(parser, name);
    getOp = Op_code::get_global_;
    setOp = Op_code::set_global_;
  }

  if (canAssign && advance_if_match(parser, scanner, Token_type::equal_)) {
    expression(parser, scanner);
    emitBytes(name, parser, setOp, arg);
  } else {
    emitBytes(name, parser, getOp, arg);
  }
}
static void variable(const Token& token, Parser& parser, Scanner& scanner, bool canAssign) {
  namedVariable(parser, scanner, token, canAssign);
}
static Token syntheticToken(const char* text) {
  Token token;
  token.lexeme_begin = text;
  token.lexeme_end = text + strlen(text);
  return token;
}
static void super_(const Token& token, Parser& parser, Scanner& scanner, bool /*canAssign*/) {
  if (currentClass == NULL) {
    throw std::runtime_error{
        "[Line " + std::to_string(token.line) +
        "] Error at '" + std::string{token.lexeme_begin, token.lexeme_end} +
        "': Cannot use 'super' outside of a class.\n"
    };
  } else if (!currentClass->hasSuperclass) {
    throw std::runtime_error{
        "[Line " + std::to_string(token.line) +
        "] Error at '" + std::string{token.lexeme_begin, token.lexeme_end} +
        "': Cannot use 'super' in a class with no superclass.\n"
    };
  }

  consume(parser, scanner, Token_type::dot_, "Expected '.' after 'super'.");
  auto parser_previous_token = consume(parser, scanner, Token_type::identifier_, "Expected superclass method name.");
  uint8_t name = identifierConstant(parser, parser_previous_token);

  namedVariable(parser, scanner, syntheticToken("this"), false);
  if (advance_if_match(parser, scanner, Token_type::left_paren_)) {
    uint8_t argCount = argumentList(parser, scanner);
    namedVariable(parser, scanner, syntheticToken("super"), false);
    emitBytes(parser_previous_token, parser, Op_code::super_invoke_, name);
    emitByte(parser_previous_token, parser, argCount);
  } else {
    namedVariable(parser, scanner, syntheticToken("super"), false);
    emitBytes(parser_previous_token, parser, Op_code::get_super_, name);
  }
}
static void this_(const Token& token, Parser& parser, Scanner& scanner, bool /*canAssign*/) {
  if (currentClass == NULL) {
    throw std::runtime_error{
        "[Line " + std::to_string(token.line) +
        "] Error at '" + std::string{token.lexeme_begin, token.lexeme_end} +
        "': Cannot use 'this' outside of a class.\n"
    };
  }
  variable(token, parser, scanner, false);
} // [this]
static void unary(const Token& token, Parser& parser, Scanner& scanner, bool /*canAssign*/) {
  Token_type operatorType = token.type;

  // Compile the operand.
  parsePrecedence(parser, scanner, Precedence::unary_);

  // Emit the operator instruction.
  switch (operatorType) {
    case Token_type::bang_: emitByte(token, parser, Op_code::not_); break;
    case Token_type::minus_: emitByte(token, parser, Op_code::negate_); break;
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
  auto current_token = parser.current;
  ParseFn prefixRule = getRule(current_token.type)->prefix;
  parser.current = scanToken(scanner);
  if (prefixRule == NULL) {
    throw std::runtime_error{
        "[Line " + std::to_string(current_token.line) +
        "] Error at '" + std::string{current_token.lexeme_begin, current_token.lexeme_end} +
        "': Expected expression.\n"
    };
  }

  bool canAssign = precedence <= Precedence::assignment_;
  prefixRule(current_token, parser, scanner, canAssign);

  while (precedence <= getRule(parser.current.type)->precedence) {
    auto current_token = parser.current;
    ParseFn infixRule = getRule(current_token.type)->infix;
    parser.current = scanToken(scanner);
    infixRule(current_token, parser, scanner, canAssign);
  }

  auto next_current_token = parser.current;
  if (canAssign && advance_if_match(parser, scanner, Token_type::equal_)) {
    throw std::runtime_error{
        "[Line " + std::to_string(next_current_token.line) +
        "] Error at '" + std::string{next_current_token.lexeme_begin, next_current_token.lexeme_end} +
        "': Invalid assignment target."
    };
  }
}
static ParseRule* getRule(Token_type type) {
  return &rules[static_cast<int>(type)];
}
static void expression(Parser& parser, Scanner& scanner) {
  parsePrecedence(parser, scanner, Precedence::assignment_);
}
static void block(Parser& parser, Scanner& scanner) {
  while (parser.current.type != Token_type::right_brace_ && parser.current.type != Token_type::eof_) {
    consume_declaration(parser, scanner);
  }

  consume(parser, scanner, Token_type::right_brace_, "Expected '}' after block.");
}
static void function(const Token& token, Parser& parser, Scanner& scanner, Function_type type) {
  Function_compiler compiler {type, token, parser, global_current->deferred_heap_, global_current->interned_strings_};
  beginScope(); // [no-end-scope]

  // Compile the parameter list.
  consume(parser, scanner, Token_type::left_paren_, "Expected '(' after function name.");
  if (parser.current.type != Token_type::right_paren_) {
    do {
      auto param_name_token = parser.current;
      global_current->function->arity++;
      if (global_current->function->arity > 255) {
        throw std::runtime_error{
            "[Line " + std::to_string(param_name_token.line) +
            "] Error at '" + std::string{param_name_token.lexeme_begin, param_name_token.lexeme_end} +
            "': Cannot have more than 255 parameters."
        };
      }

      uint8_t paramConstant = parseVariable(parser, scanner, "Expect parameter name.");
      defineVariable(param_name_token, parser, paramConstant);
    } while (advance_if_match(parser, scanner, Token_type::comma_));
  }
  consume(parser, scanner, Token_type::right_paren_, "Expected ')' after parameters.");

  // The body.
  consume(parser, scanner, Token_type::left_brace_, "Expected '{' before function body.");
  block(parser, scanner);

  // Create the function object.
  ObjFunction* function = endCompiler(parser.current, parser);
  emitBytes(parser.current, parser, Op_code::closure_, makeConstant(token, parser, Value{function}));

  for (int i = 0; i < function->upvalueCount; i++) {
    emitByte(token, parser, compiler.upvalues[i].isLocal ? 1 : 0);
    emitByte(token, parser, compiler.upvalues[i].index);
  }
}
static void method(Parser& parser, Scanner& scanner) {
  auto method_name_token = consume(parser, scanner, Token_type::identifier_, "Expected method name.");
  uint8_t constant = identifierConstant(parser, method_name_token);

  Function_type type = Function_type::method_;
  if ((method_name_token.lexeme_end - method_name_token.lexeme_begin) == 4 &&
      memcmp(method_name_token.lexeme_begin, "init", 4) == 0) {
    type = Function_type::initializer_;
  }

  function(method_name_token, parser, scanner, type);
  emitBytes(method_name_token, parser, Op_code::method_, constant);
}
static void consume_class_declaration(Parser& parser, Scanner& scanner) {
  Token className = consume(parser, scanner, Token_type::identifier_, "Expected class name.");
  uint8_t nameConstant = identifierConstant(parser, className);
  declareVariable(className, parser);

  emitBytes(className, parser, Op_code::class_, nameConstant);
  defineVariable(className, parser, nameConstant);

  ClassCompiler classCompiler;
  classCompiler.name = className;
  classCompiler.hasSuperclass = false;
  classCompiler.enclosing = currentClass;
  currentClass = &classCompiler;

  if (advance_if_match(parser, scanner, Token_type::less_)) {
    auto superclass_name_token = consume(parser, scanner, Token_type::identifier_, "Expected superclass name.");
    variable(superclass_name_token, parser, scanner, false);

    if (identifiersEqual(className, superclass_name_token)) {
      throw std::runtime_error{
          "[Line " + std::to_string(superclass_name_token.line) +
          "] Error at '" + std::string{superclass_name_token.lexeme_begin, superclass_name_token.lexeme_end} +
          "': A class cannot inherit from itself."
      };
    }

    beginScope();
    addLocal(parser, syntheticToken("super"));
    defineVariable(superclass_name_token, parser, 0);

    namedVariable(parser, scanner, className, false);
    emitByte(superclass_name_token, parser, Op_code::inherit_);
    classCompiler.hasSuperclass = true;
  }

  namedVariable(parser, scanner, className, false);
  consume(parser, scanner, Token_type::left_brace_, "Expected '{' before class body.");
  while (parser.current.type != Token_type::right_brace_ && parser.current.type != Token_type::eof_) {
    method(parser, scanner);
  }
  auto right_brace_token = consume(parser, scanner, Token_type::right_brace_, "Expected '}' after class body.");
  emitByte(right_brace_token, parser, Op_code::pop_);

  if (classCompiler.hasSuperclass) {
    endScope(className, parser);
  }

  currentClass = currentClass->enclosing;
}
static void funDeclaration(Parser& parser, Scanner& scanner) {
  auto fun_name_token = parser.current;
  uint8_t global = parseVariable(parser, scanner, "Expect function name.");
  markInitialized();
  function(fun_name_token, parser, scanner, Function_type::function_);
  defineVariable(fun_name_token, parser, global);
}
static void varDeclaration(Parser& parser, Scanner& scanner) {
  auto var_name_token = parser.current;
  uint8_t global = parseVariable(parser, scanner, "Expect variable name.");

  if (advance_if_match(parser, scanner, Token_type::equal_)) {
    expression(parser, scanner);
  } else {
    emitByte(var_name_token, parser, Op_code::nil_);
  }
  consume(parser, scanner, Token_type::semicolon_, "Expected ';' after variable declaration.");

  defineVariable(var_name_token, parser, global);
}
static void expressionStatement(Parser& parser, Scanner& scanner) {
  expression(parser, scanner);
  auto token = consume(parser, scanner, Token_type::semicolon_, "Expect ';' after expression.");
  emitByte(token, parser, Op_code::pop_);
}
static void forStatement(Parser& parser, Scanner& scanner) {
  beginScope();

  consume(parser, scanner, Token_type::left_paren_, "Expected '(' after 'for'.");
  if (advance_if_match(parser, scanner, Token_type::semicolon_)) {
    // No initializer.
  } else if (advance_if_match(parser, scanner, Token_type::var_)) {
    varDeclaration(parser, scanner);
  } else {
    expressionStatement(parser, scanner);
  }

  int loopStart = global_current->function->chunk.code.size();

  int exitJump = -1;
  if (!advance_if_match(parser, scanner, Token_type::semicolon_)) {
    auto token = parser.current;
    expression(parser, scanner);
    consume(parser, scanner, Token_type::semicolon_, "Expected ';' after loop condition.");

    // Jump out of the loop if the condition is false.
    exitJump = emitJump(token, parser, Op_code::jump_if_false_);
    emitByte(token, parser, Op_code::pop_); // Condition.
  }

  auto right_param_token = parser.current;
  if (!advance_if_match(parser, scanner, Token_type::right_paren_)) {
    int bodyJump = emitJump(right_param_token, parser, Op_code::jump_);

    int incrementStart = global_current->function->chunk.code.size();
    auto token = parser.current;
    expression(parser, scanner);
    emitByte(token, parser, Op_code::pop_);
    consume(parser, scanner, Token_type::right_paren_, "Expected ')' after for clauses.");

    emitLoop(token, parser, loopStart);
    loopStart = incrementStart;
    patchJump(token, parser, bodyJump);
  }

  auto token = parser.current;
  statement(parser, scanner);

  emitLoop(token, parser, loopStart);

  if (exitJump != -1) {
    patchJump(token, parser, exitJump);
    emitByte(token, parser, Op_code::pop_); // Condition.
  }

  endScope(token, parser);
}
static void ifStatement(Parser& parser, Scanner& scanner) {
  consume(parser, scanner, Token_type::left_paren_, "Expected '(' after 'if'.");
  expression(parser, scanner);
  auto right_param_token = consume(parser, scanner, Token_type::right_paren_, "Expected ')' after condition."); // [paren]

  int thenJump = emitJump(right_param_token, parser, Op_code::jump_if_false_);
  emitByte(right_param_token, parser, Op_code::pop_);
  auto token = parser.current;
  statement(parser, scanner);

  int elseJump = emitJump(token, parser, Op_code::jump_);

  patchJump(token, parser, thenJump);
  emitByte(token, parser, Op_code::pop_);

  auto next_current_token = parser.current;
  if (advance_if_match(parser, scanner, Token_type::else_)) statement(parser, scanner);
  patchJump(next_current_token, parser, elseJump);
}
static void printStatement(Parser& parser, Scanner& scanner) {
  expression(parser, scanner);
  auto token = consume(parser, scanner, Token_type::semicolon_, "Expected ';' after value.");
  emitByte(token, parser, Op_code::print_);
}
static void returnStatement(const Token& token, Parser& parser, Scanner& scanner) {
  if (global_current->type == Function_type::script_) {
    throw std::runtime_error{
        "[Line " + std::to_string(token.line) +
        "] Error at '" + std::string{token.lexeme_begin, token.lexeme_end} +
        "': Cannot return from top-level code.\n"
    };
  }

  if (advance_if_match(parser, scanner, Token_type::semicolon_)) {
    emitReturn(token, parser);
  } else {
    if (global_current->type == Function_type::initializer_) {
      throw std::runtime_error{
          "[Line " + std::to_string(token.line) +
          "] Error at '" + std::string{token.lexeme_begin, token.lexeme_end} +
          "': Cannot return a value from an initializer.\n"
      };
    }

    expression(parser, scanner);
    consume(parser, scanner, Token_type::semicolon_, "Expected ';' after return value.");
    emitByte(token, parser, Op_code::return_);
  }
}
static void whileStatement(Parser& parser, Scanner& scanner) {
  int loopStart = global_current->function->chunk.code.size();

  consume(parser, scanner, Token_type::left_paren_, "Expected '(' after 'while'.");
  expression(parser, scanner);
  auto right_param_token = consume(parser, scanner, Token_type::right_paren_, "Expected ')' after condition.");

  int exitJump = emitJump(right_param_token, parser, Op_code::jump_if_false_);

  emitByte(right_param_token, parser, Op_code::pop_);
  auto token = parser.current;
  statement(parser, scanner);

  emitLoop(token, parser, loopStart);

  patchJump(token, parser, exitJump);
  emitByte(token, parser, Op_code::pop_);
}
static void synchronize(Parser& parser, Scanner& scanner) {
  while (parser.current.type != Token_type::eof_) {
    if (advance_if_match(parser, scanner, Token_type::semicolon_)) return;

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

    parser.current = scanToken(scanner);
  }
}
static void consume_declaration(Parser& parser, Scanner& scanner) {
  // try {
    if (advance_if_match(parser, scanner, Token_type::class_)) {
      consume_class_declaration(parser, scanner);
    } else if (advance_if_match(parser, scanner, Token_type::fun_)) {
      funDeclaration(parser, scanner);
    } else if (advance_if_match(parser, scanner, Token_type::var_)) {
      varDeclaration(parser, scanner);
    } else {
      statement(parser, scanner);
    }
  // } catch (const std::exception& e) {
  //   synchronize(parser, scanner);
  // }
}
static void statement(Parser& parser, Scanner& scanner) {
  auto token = parser.current;

  if (advance_if_match(parser, scanner, Token_type::print_)) {
    printStatement(parser, scanner);
  } else if (advance_if_match(parser, scanner, Token_type::for_)) {
    forStatement(parser, scanner);
  } else if (advance_if_match(parser, scanner, Token_type::if_)) {
    ifStatement(parser, scanner);
  } else if (advance_if_match(parser, scanner, Token_type::return_)) {
    returnStatement(token, parser, scanner);
  } else if (advance_if_match(parser, scanner, Token_type::while_)) {
    whileStatement(parser, scanner);
  } else if (advance_if_match(parser, scanner, Token_type::left_brace_)) {
    beginScope();
    block(parser, scanner);
    endScope(token, parser);
  } else {
    expressionStatement(parser, scanner);
  }
}

ObjFunction* compile(const std::string& source, Deferred_heap& deferred_heap, Interned_strings& interned_strings) {
  Scanner scanner {source};
  Parser parser;

  // Side-effect: sets global "current" compiler
  Function_compiler compiler {Function_type::script_, Token{}, parser, deferred_heap, interned_strings};

  deferred_heap.mark_roots_callbacks.push_back([&] () {
    Function_compiler* compiler = global_current;
    while (compiler) {
      deferred_heap.mark(*compiler->function);
      compiler = compiler->enclosing;
    }
  });

  parser.current = scanToken(scanner);
  while (parser.current.type != Token_type::eof_) {
    consume_declaration(parser, scanner);
  }

  ObjFunction* function = endCompiler(parser.current, parser);
  return parser.hadError ? NULL : function;
}
