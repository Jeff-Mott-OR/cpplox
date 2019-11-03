#include "compiler.hpp"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <vector>

#include <boost/lexical_cast.hpp>

#include "debug.hpp"

using std::cout;
using std::for_each;
using std::runtime_error;
using std::setw;
using std::string;
using std::to_string;
using std::vector;

using boost::lexical_cast;

using namespace motts::lox;

class Compiler {
    public:
        struct Parser {
          Token current;
          Token previous;
          bool hadError;
          bool panicMode;
        };

        enum class Precedence {
          none,
          assignment,  // =
          or_,         // or
          and_,        // and
          equality,    // == !=
          comparison,  // < > <= >=
          term,        // + -
          factor,      // * /
          unary,       // ! -
          call,        // . () []
          primary
        };

        using ParseFn = void (Compiler::*)();

        struct ParseRule {
          ParseFn prefix;
          ParseFn infix;
          Precedence precedence;
        };

        Scanner scanner_;
        Chunk chunk_;
        Chunk* compiling_chunk;
        Parser parser_;
        vector<ParseRule> rules_;

        Chunk& current_chunk() {
          return *compiling_chunk;
        }

        int makeConstant(Value value) {
          compiling_chunk->constants.push_back(value);
          const auto constant_offset = compiling_chunk->constants.size() - 1;
          if (constant_offset > UINT8_MAX) {
            throw Compiler_error{parser_.previous, "Too many constants in one chunk."};
          }

          return constant_offset;
        }

        void emitByte(int byte) {
          write_chunk(current_chunk(), byte, parser_.previous.line);
        }

        void emitByte(Op_code opcode) {
            emitByte(static_cast<int>(opcode));
        }

        void emitBytes(int byte1, int byte2) {
          emitByte(byte1);
          emitByte(byte2);
        }

        void emitConstant(Value value) {
          emitBytes(static_cast<int>(Op_code::constant), makeConstant(value));
        }

        void consume(Token_type type, const string& message) {
          if (parser_.current.type == type) {
            advance();
            return;
          }

          throw Compiler_error{parser_.current, message};
        }

        void advance() {
          parser_.previous = parser_.current;

          while (true) {
            try {
                parser_.current = scanner_.scan_token();
            } catch (const Scanner_error& error) {
                // TODO Accumulate errors, and continue scanning tokens until non-error
                throw Compiler_error{parser_.current, error.what()};
            }

            break;
          }
        }

        void expression() {
          parsePrecedence(Precedence::assignment);
        }

        void grouping() {
          expression();
          consume(Token_type::right_paren, "Expected ')' after expression.");
        }

        void number() {
          const auto value = lexical_cast<int>(string{parser_.previous.begin, parser_.previous.end});
          emitConstant(value);
        }

        void unary() {
          const auto operatorType = parser_.previous.type;

          // Compile the operand.
          parsePrecedence(Precedence::unary);

          // Emit the operator instruction.
          switch (operatorType) {
            case Token_type::minus:
                emitByte(static_cast<int>(Op_code::negate));
                break;
            default:
              throw Compiler_error{parser_.previous, "Unreachable."};
          }
        }

        void binary() {
          // Remember the operator.
          const auto operatorType = parser_.previous.type;

          // Compile the right operand.
          const auto rule = getRule(operatorType);
          parsePrecedence(static_cast<Precedence>(static_cast<int>(rule.precedence) + 1));

          // Emit the operator instruction.
          switch (operatorType) {
            case Token_type::plus:          emitByte(Op_code::add); break;
            case Token_type::minus:         emitByte(Op_code::subtract); break;
            case Token_type::star:          emitByte(Op_code::multiply); break;
            case Token_type::slash:         emitByte(Op_code::divide); break;
            default:
              throw Compiler_error{parser_.previous, "Unreachable."};
          }
        }

        void parsePrecedence(Precedence precedence) {
          advance();
          const auto prefixRule = getRule(parser_.previous.type).prefix;
          if (!prefixRule) {
            throw Compiler_error{parser_.previous, "Expected expression."};
          }

          (this->*prefixRule)();

          while (static_cast<int>(precedence) <= static_cast<int>(getRule(parser_.current.type).precedence)) {
            advance();
            const auto infixRule = getRule(parser_.previous.type).infix;
            (this->*infixRule)();
          }
        }

        const ParseRule& getRule(Token_type type) {
          return rules_.at(static_cast<int>(type));
        }

    public:
        Compiler(const string& source) :
            scanner_ {source},
            compiling_chunk {&chunk_},
            // TODO Document that order matters
            rules_ {
              { &Compiler::grouping, nullptr,    Precedence::none },       // TOKEN_LEFT_PAREN
              { nullptr,     nullptr,    Precedence::none },       // TOKEN_RIGHT_PAREN
              { nullptr,     nullptr,    Precedence::none },       // TOKEN_LEFT_BRACE
              { nullptr,     nullptr,    Precedence::none },       // TOKEN_RIGHT_BRACE
              { nullptr,     nullptr,    Precedence::none },       // TOKEN_COMMA
              { nullptr,     nullptr,    Precedence::none },       // TOKEN_DOT
              { &Compiler::unary,    &Compiler::binary,  Precedence::term },       // TOKEN_MINUS
              { nullptr,     &Compiler::binary,  Precedence::term },       // TOKEN_PLUS
              { nullptr,     nullptr,    Precedence::none },       // TOKEN_SEMICOLON
              { nullptr,     &Compiler::binary,  Precedence::factor },     // TOKEN_SLASH
              { nullptr,     &Compiler::binary,  Precedence::factor },     // TOKEN_STAR
              { nullptr,     nullptr,    Precedence::none },       // TOKEN_BANG
              { nullptr,     nullptr,    Precedence::none },       // TOKEN_BANG_EQUAL
              { nullptr,     nullptr,    Precedence::none },       // TOKEN_EQUAL
              { nullptr,     nullptr,    Precedence::none },       // TOKEN_EQUAL_EQUAL
              { nullptr,     nullptr,    Precedence::none },       // TOKEN_GREATER
              { nullptr,     nullptr,    Precedence::none },       // TOKEN_GREATER_EQUAL
              { nullptr,     nullptr,    Precedence::none },       // TOKEN_LESS
              { nullptr,     nullptr,    Precedence::none },       // TOKEN_LESS_EQUAL
              { nullptr,     nullptr,    Precedence::none },       // TOKEN_IDENTIFIER
              { nullptr,     nullptr,    Precedence::none },       // TOKEN_STRING
              { &Compiler::number,   nullptr,    Precedence::none },       // TOKEN_NUMBER
              { nullptr,     nullptr,    Precedence::none },       // TOKEN_AND
              { nullptr,     nullptr,    Precedence::none },       // TOKEN_CLASS
              { nullptr,     nullptr,    Precedence::none },       // TOKEN_ELSE
              { nullptr,     nullptr,    Precedence::none },       // TOKEN_FALSE
              { nullptr,     nullptr,    Precedence::none },       // TOKEN_FOR
              { nullptr,     nullptr,    Precedence::none },       // TOKEN_FUN
              { nullptr,     nullptr,    Precedence::none },       // TOKEN_IF
              { nullptr,     nullptr,    Precedence::none },       // TOKEN_NIL
              { nullptr,     nullptr,    Precedence::none },       // TOKEN_OR
              { nullptr,     nullptr,    Precedence::none },       // TOKEN_PRINT
              { nullptr,     nullptr,    Precedence::none },       // TOKEN_RETURN
              { nullptr,     nullptr,    Precedence::none },       // TOKEN_SUPER
              { nullptr,     nullptr,    Precedence::none },       // TOKEN_THIS
              { nullptr,     nullptr,    Precedence::none },       // TOKEN_TRUE
              { nullptr,     nullptr,    Precedence::none },       // TOKEN_VAR
              { nullptr,     nullptr,    Precedence::none },       // TOKEN_WHILE
              { nullptr,     nullptr,    Precedence::none },       // TOKEN_ERROR
              { nullptr,     nullptr,    Precedence::none },       // TOKEN_EOF
            }
        {
        }
};

Compiler_error::Compiler_error(const Token& token, const string& what) :
    runtime_error{
        "[Line " + to_string(token.line) + "] Error" +
        (
            token.type == Token_type::eof ?
                " at end" :
                " at '" + string{token.begin, token.end} + "'"
        ) +
        ": " + what
    }
{}

namespace motts { namespace lox {
    Chunk compile(const string& source) {
        Compiler compiler {source};

        compiler.advance();
        compiler.expression();
        compiler.consume(Token_type::eof, "Expect end of expression.");

        disassemble_chunk(compiler.current_chunk(), "code");

        compiler.emitByte(Op_code::return_);

        return compiler.chunk_;
    }
}}
