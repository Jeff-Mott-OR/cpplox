#include <stdio.h>
#include <string.h>

#include "common.hpp"
#include "scanner.hpp"

static bool isAlpha(char c) {
  return (c >= 'a' && c <= 'z') ||
         (c >= 'A' && c <= 'Z') ||
          c == '_';
}
static bool isDigit(char c) {
  return c >= '0' && c <= '9';
}
static bool isAtEnd(Scanner& scanner) {
  return *scanner.current == '\0';
}
static char advance(Scanner& scanner) {
  scanner.current++;
  return scanner.current[-1];
}
static char peek(Scanner& scanner) {
  return *scanner.current;
}
static char peekNext(Scanner& scanner) {
  if (isAtEnd(scanner)) return '\0';
  return scanner.current[1];
}
static bool match(Scanner& scanner, char expected) {
  if (isAtEnd(scanner)) return false;
  if (*scanner.current != expected) return false;

  scanner.current++;
  return true;
}
static Token makeToken(Scanner& scanner, Token_type type) {
  Token token;
  token.type = type;
  token.start = scanner.start;
  token.length = (int)(scanner.current - scanner.start);
  token.line = scanner.line;

  return token;
}
static Token errorToken(Scanner& scanner, const char* message) {
  Token token;
  token.type = Token_type::error_;
  token.start = message;
  token.length = (int)strlen(message);
  token.line = scanner.line;

  return token;
}
static void skipWhitespace(Scanner& scanner) {
  for (;;) {
    char c = peek(scanner);
    switch (c) {
      case ' ':
      case '\r':
      case '\t':
        advance(scanner);
        break;

      case '\n':
        scanner.line++;
        advance(scanner);
        break;

      case '/':
        if (peekNext(scanner) == '/') {
          // A comment goes until the end of the line.
          while (peek(scanner) != '\n' && !isAtEnd(scanner)) advance(scanner);
        } else {
          return;
        }
        break;

      default:
        return;
    }
  }
}
static Token_type checkKeyword(Scanner& scanner, int start, int length, const char* rest, Token_type type) {
  if (scanner.current - scanner.start == start + length &&
      memcmp(scanner.start + start, rest, length) == 0) {
    return type;
  }

  return Token_type::identifier_;
}
static Token_type identifierType(Scanner& scanner)
{
  switch (scanner.start[0]) {
    case 'a': return checkKeyword(scanner, 1, 2, "nd", Token_type::and_);
    case 'c': return checkKeyword(scanner, 1, 4, "lass", Token_type::class_);
    case 'e': return checkKeyword(scanner, 1, 3, "lse", Token_type::else_);
    case 'f':
      if (scanner.current - scanner.start > 1) {
        switch (scanner.start[1]) {
          case 'a': return checkKeyword(scanner, 2, 3, "lse", Token_type::false_);
          case 'o': return checkKeyword(scanner, 2, 1, "r", Token_type::for_);
          case 'u': return checkKeyword(scanner, 2, 1, "n", Token_type::fun_);
        }
      }
      break;
    case 'i': return checkKeyword(scanner, 1, 1, "f", Token_type::if_);
    case 'n': return checkKeyword(scanner, 1, 2, "il", Token_type::nil_);
    case 'o': return checkKeyword(scanner, 1, 1, "r", Token_type::or_);
    case 'p': return checkKeyword(scanner, 1, 4, "rint", Token_type::print_);
    case 'r': return checkKeyword(scanner, 1, 5, "eturn", Token_type::return_);
    case 's': return checkKeyword(scanner, 1, 4, "uper", Token_type::super_);
    case 't':
      if (scanner.current - scanner.start > 1) {
        switch (scanner.start[1]) {
          case 'h': return checkKeyword(scanner, 2, 2, "is", Token_type::this_);
          case 'r': return checkKeyword(scanner, 2, 2, "ue", Token_type::true_);
        }
      }
      break;
    case 'v': return checkKeyword(scanner, 1, 2, "ar", Token_type::var_);
    case 'w': return checkKeyword(scanner, 1, 4, "hile", Token_type::while_);
  }

  return Token_type::identifier_;
}
static Token identifier(Scanner& scanner) {
  while (isAlpha(peek(scanner)) || isDigit(peek(scanner))) advance(scanner);

  return makeToken(scanner, identifierType(scanner));
}
static Token number(Scanner& scanner) {
  while (isDigit(peek(scanner))) advance(scanner);

  // Look for a fractional part.
  if (peek(scanner) == '.' && isDigit(peekNext(scanner))) {
    // Consume the ".".
    advance(scanner);

    while (isDigit(peek(scanner))) advance(scanner);
  }

  return makeToken(scanner, Token_type::number_);
}
static Token string(Scanner& scanner) {
  while (peek(scanner) != '"' && !isAtEnd(scanner)) {
    if (peek(scanner) == '\n') scanner.line++;
    advance(scanner);
  }

  if (isAtEnd(scanner)) return errorToken(scanner, "Unterminated string.");

  // The closing quote.
  advance(scanner);
  return makeToken(scanner, Token_type::string_);
}
Token scanToken(Scanner& scanner) {
  skipWhitespace(scanner);

  scanner.start = scanner.current;

  if (isAtEnd(scanner)) return makeToken(scanner, Token_type::eof_);

  char c = advance(scanner);

  if (isAlpha(c)) return identifier(scanner);
  if (isDigit(c)) return number(scanner);

  switch (c) {
    case '(': return makeToken(scanner, Token_type::left_paren_);
    case ')': return makeToken(scanner, Token_type::right_paren_);
    case '{': return makeToken(scanner, Token_type::left_brace_);
    case '}': return makeToken(scanner, Token_type::right_brace_);
    case ';': return makeToken(scanner, Token_type::semicolon_);
    case ',': return makeToken(scanner, Token_type::comma_);
    case '.': return makeToken(scanner, Token_type::dot_);
    case '-': return makeToken(scanner, Token_type::minus_);
    case '+': return makeToken(scanner, Token_type::plus_);
    case '/': return makeToken(scanner, Token_type::slash_);
    case '*': return makeToken(scanner, Token_type::star_);
    case '!':
      return makeToken(scanner, match(scanner, '=') ? Token_type::bang_equal_ : Token_type::bang_);
    case '=':
      return makeToken(scanner, match(scanner, '=') ? Token_type::equal_equal_ : Token_type::equal_);
    case '<':
      return makeToken(scanner, match(scanner, '=') ? Token_type::less_equal_ : Token_type::less_);
    case '>':
      return makeToken(scanner, match(scanner, '=') ?
                       Token_type::greater_equal_ : Token_type::greater_);

    case '"': return string(scanner);
  }

  return errorToken(scanner, "Unexpected character.");
}
