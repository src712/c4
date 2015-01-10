#ifndef C4_TOKEN_H
#define C4_TOKEN_H

#include "llvm/ADT/StringRef.h"

#include <string>
#include <vector>
#include <iosfwd>

namespace c4 {

// If you ever consider reordering those, remember to also reorder the
// token.cc table. Really.
enum class token_type : int {
    T_EOF = 0, /* EOF macro is around */
    INVALID, INTEGER_CONSTANT, CHARACTER_CONSTANT, 
    STRING_LITERAL, COMMENT, IDENTIFIER, 
  // punctuators
    PCTR_LBRACKET, PCTR_RBRACKET, PCTR_LPAREN, PCTR_RPAREN, PCTR_LBRACE, 
    PCTR_RBRACE, PCTR_COLON, PCTR_SEMICOLON, PCTR_TILDE, PCTR_COMMA, 
    PCTR_DOT, PCTR_STAR, PCTR_ASSIGN, PCTR_HASH, PCTR_BIT_AND, PCTR_PLUS, 
    PCTR_MINUS, PCTR_BANG, PCTR_SLASH, PCTR_MOD, PCTR_LESS, PCTR_GREATER, 
    PCTR_POWER, PCTR_BIT_OR, PCTR_QUESTIONMARK, PCTR_DEREF, PCTR_INCREMENT, 
    PCTR_DECREMENT, PCTR_HASHASH, PCTR_NOT_EQUAL, PCTR_LESS_THAN, PCTR_GREATER_THAN, 
    PCTR_EQUAL, PCTR_AND, PCTR_OR, PCTR_MULT_ASSIGN, PCTR_DIV_ASSIGN, PCTR_MOD_ASSIGN, 
    PCTR_PLUS_ASSIGN, PCTR_MINUS_ASSIGN, PCTR_BIT_AND_ASSIGN,
    PCTR_POWER_ASSIGN, PCTR_BIT_OR_ASSIGN, PCTR_SHIFT_LEFT, PCTR_SHIFT_RIGHT, 
    PCTR_VARARGS, PCTR_SHIFT_LEFT_ASSIGN, PCTR_SHIFT_RIGHT_ASSIGN,
  // keywords
    KWD_AUTO, KWD_BREAK, KWD_CASE,
    KWD_CHAR, KWD_CONST, KWD_CONTINUE, KWD_DEFAULT, KWD_DO, KWD_DOUBLE,
    KWD_ELSE, KWD_ENUM, KWD_EXTERN, KWD_FLOAT, KWD_FOR, KWD_GOTO, KWD_IF,
    KWD_INLINE, KWD_INT, KWD_LONG, KWD_REGISTER, KWD_RESTRICT, KWD_RETURN, KWD_SHORT, 
    KWD_SIGNED, KWD_SIZEOF, KWD_STATIC, KWD_STRUCT, KWD_SWITCH, KWD_TYPEDEF, 
    KWD_UNION, KWD_UNSIGNED, KWD_VOID, KWD_VOLATILE, KWD_WHILE, 
    KWD__ALIGNAS, KWD__ALIGNOF, KWD__ATOMIC, KWD__BOOL, KWD__COMPLEX, KWD__GENERIC,
    KWD__IMAGINARY, KWD__NORETURN, KWD__STATIC_ASSERT, KWD__THREAD_LOCAL
};

} // c4

namespace c4 {

const char* token_to_string(const token_type&);

struct token
{
  token(token_type type, const llvm::StringRef& data)
    : type(type), data(data) {}
  token(token_type type, const llvm::StringRef& data, std::vector<std::string> errors)
    : type(type), data(data), errors(std::move(errors)) {}

  token(token&&) = default;
  token& operator=(token&&) = default;
  token(const token&) = default;
  token& operator=(const token&) = default;

  token_type type;
  llvm::StringRef data;
  std::vector<std::string> errors;
};

std::ostream& operator<<(std::ostream& o, const token& t);

} // c4

#endif /* C4_TOKEN_H */
