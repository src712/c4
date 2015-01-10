#include "token.h"
#include <ostream>
#include <cassert>

namespace c4 {

const char* token_to_string(const token_type& t) {
  static std::pair<token_type, const char*> token_str_map[] = {
    {token_type::T_EOF, "EOF"},
    {token_type::INVALID, "INVALID"}, {token_type::INTEGER_CONSTANT, "integer constant"}, 
    {token_type::CHARACTER_CONSTANT, "character constant"}, {token_type::STRING_LITERAL, "string-literal"}, 
    {token_type::COMMENT, "comment"}, {token_type::IDENTIFIER, "identifier"},
    // punctuators
    {token_type::PCTR_LBRACKET, "["}, {token_type::PCTR_RBRACKET, "]"}, 
    {token_type::PCTR_LPAREN, "("}, {token_type::PCTR_RPAREN, ")"}, {token_type::PCTR_LBRACE, "{"},
    {token_type::PCTR_RBRACE, "}"}, {token_type::PCTR_COLON, ":"}, {token_type::PCTR_SEMICOLON, ";"}, 
    {token_type::PCTR_TILDE, "~"}, {token_type::PCTR_COMMA, ","}, {token_type::PCTR_DOT, "."}, 
    {token_type::PCTR_STAR, "*"}, {token_type::PCTR_ASSIGN, "="}, {token_type::PCTR_HASH, "#"}, 
    {token_type::PCTR_BIT_AND, "&"}, {token_type::PCTR_PLUS, "+"}, {token_type::PCTR_MINUS, "-"}, 
    {token_type::PCTR_BANG, "!"}, {token_type::PCTR_SLASH, "/"}, {token_type::PCTR_MOD, "%"}, 
    {token_type::PCTR_LESS, "<"}, {token_type::PCTR_GREATER, ">"}, {token_type::PCTR_POWER, "^"}, 
    {token_type::PCTR_BIT_OR, "|"}, {token_type::PCTR_QUESTIONMARK, "?"}, {token_type::PCTR_DEREF, "->"}, 
    {token_type::PCTR_INCREMENT, "++"}, {token_type::PCTR_DECREMENT, "--"}, {token_type::PCTR_HASHASH, "##"}, 
    {token_type::PCTR_NOT_EQUAL, "!="}, {token_type::PCTR_LESS_THAN, "<="}, {token_type::PCTR_GREATER_THAN, ">="}, 
    {token_type::PCTR_EQUAL, "=="}, {token_type::PCTR_AND, "&&"}, {token_type::PCTR_OR, "||"}, 
    {token_type::PCTR_MULT_ASSIGN, "*="}, {token_type::PCTR_DIV_ASSIGN, "/="}, {token_type::PCTR_MOD_ASSIGN, "%="}, 
    {token_type::PCTR_PLUS_ASSIGN, "+="}, {token_type::PCTR_MINUS_ASSIGN, "-="}, 
    {token_type::PCTR_BIT_AND_ASSIGN, "&="}, {token_type::PCTR_POWER_ASSIGN, "^="}, 
    {token_type::PCTR_BIT_OR_ASSIGN, "|="}, {token_type::PCTR_SHIFT_LEFT, "<<"}, 
    {token_type::PCTR_SHIFT_RIGHT, ">>"}, {token_type::PCTR_VARARGS, "..."}, 
    {token_type::PCTR_SHIFT_LEFT_ASSIGN, "<<="}, {token_type::PCTR_SHIFT_RIGHT_ASSIGN, ">>="},
    // keywords
    {token_type::KWD_AUTO, "auto"}, {token_type::KWD_BREAK, "break"}, {token_type::KWD_CASE, "case"},
    {token_type::KWD_CHAR, "char"}, {token_type::KWD_CONST, "const"}, {token_type::KWD_CONTINUE, "continue"},
    {token_type::KWD_DEFAULT, "default"}, {token_type::KWD_DO, "do"}, {token_type::KWD_DOUBLE, "double"},
    {token_type::KWD_ELSE, "else"}, {token_type::KWD_ENUM, "enum"}, {token_type::KWD_EXTERN, "extern"},
    {token_type::KWD_FLOAT, "float"}, {token_type::KWD_FOR, "for"}, {token_type::KWD_GOTO, "goto"},
    {token_type::KWD_IF, "if"}, {token_type::KWD_INLINE, "inline"}, {token_type::KWD_INT, "int"},
    {token_type::KWD_LONG, "long"}, {token_type::KWD_REGISTER, "register"},
    {token_type::KWD_RESTRICT, "restrict"}, {token_type::KWD_RETURN, "return"}, {token_type::KWD_SHORT, "short"},
    {token_type::KWD_SIGNED, "signed"}, {token_type::KWD_SIZEOF, "sizeof"}, {token_type::KWD_STATIC, "static"},
    {token_type::KWD_STRUCT, "struct"}, {token_type::KWD_SWITCH, "switch"}, {token_type::KWD_TYPEDEF, "typedef"},
    {token_type::KWD_UNION, "union"}, {token_type::KWD_UNSIGNED, "unsigned"}, {token_type::KWD_VOID, "void"},
    {token_type::KWD_VOLATILE, "volatile"}, {token_type::KWD_WHILE, "while"}, {token_type::KWD__ALIGNAS, "_Alignas"},
    {token_type::KWD__ALIGNOF, "_Alignof"}, { token_type::KWD__ATOMIC, "_Atomic"}, {token_type::KWD__BOOL, "_Bool"},
    {token_type::KWD__COMPLEX, "_Complex"}, {token_type::KWD__GENERIC, "_Generic"},
    {token_type::KWD__IMAGINARY, "_Imaginary"}, {token_type::KWD__NORETURN, "_Noreturn"},
    {token_type::KWD__STATIC_ASSERT, "_Static_assert"}, {token_type::KWD__THREAD_LOCAL, "_Thread_local"}
  };
  return token_str_map[static_cast<int>(t)].second;
}

// internal
namespace {

const char* pctr_to_string(const token_type& t)
{
  assert(t >= token_type::PCTR_LBRACKET 
         && t <= token_type::PCTR_SHIFT_RIGHT_ASSIGN
         && "pctr_to_string: passed token that is not a punctuator");
  return token_to_string(t);
}

const char* kwd_to_string(const token_type& t)
{
  assert(t >= token_type::KWD_AUTO 
         && t <= token_type::KWD__THREAD_LOCAL
         && "kwd_to_string: passed token that is not a keyword");
  return token_to_string(t);
}

}

std::ostream& operator<<(std::ostream& o, const token& t) {
  switch(t.type) {
  case token_type::INVALID:
    o.write(t.data.data(), t.data.size());
    break;
  case token_type::INTEGER_CONSTANT:
  case token_type::CHARACTER_CONSTANT:
    o << "constant "; o.write(t.data.data(), t.data.size());
    break;
  case token_type::STRING_LITERAL:
    o << "string-literal "; o.write(t.data.data(), t.data.size());
    break;
  case token_type::COMMENT:
    o << "comment";
    break;
  case token_type::T_EOF:
    o << "EOF";
    break;
  case token_type::IDENTIFIER:
    o << "identifier "; o.write(t.data.data(), t.data.size());
    break;
    // handle digraph punctuators
  case token_type::PCTR_LBRACKET:
  case token_type::PCTR_RBRACKET:
  case token_type::PCTR_LBRACE:
  case token_type::PCTR_RBRACE:
  case token_type::PCTR_HASH:
  case token_type::PCTR_HASHASH:
    o << "punctuator "; o.write(t.data.data(), t.data.size());
    break;
    //handle punctuators generically
  case token_type::PCTR_LPAREN:
  case token_type::PCTR_RPAREN:
  case token_type::PCTR_COLON:
  case token_type::PCTR_SEMICOLON:
  case token_type::PCTR_TILDE:
  case token_type::PCTR_COMMA:
  case token_type::PCTR_DOT:
  case token_type::PCTR_STAR:
  case token_type::PCTR_ASSIGN:
  case token_type::PCTR_BIT_AND:
  case token_type::PCTR_PLUS:
  case token_type::PCTR_MINUS:
  case token_type::PCTR_BANG:
  case token_type::PCTR_SLASH:
  case token_type::PCTR_MOD:
  case token_type::PCTR_LESS:
  case token_type::PCTR_GREATER:
  case token_type::PCTR_POWER:
  case token_type::PCTR_BIT_OR:
  case token_type::PCTR_QUESTIONMARK:
  case token_type::PCTR_DEREF:
  case token_type::PCTR_INCREMENT:
  case token_type::PCTR_DECREMENT:
  case token_type::PCTR_NOT_EQUAL:
  case token_type::PCTR_LESS_THAN:
  case token_type::PCTR_GREATER_THAN:
  case token_type::PCTR_EQUAL:
  case token_type::PCTR_AND:
  case token_type::PCTR_OR:
  case token_type::PCTR_MULT_ASSIGN:
  case token_type::PCTR_DIV_ASSIGN:
  case token_type::PCTR_MOD_ASSIGN:
  case token_type::PCTR_PLUS_ASSIGN:
  case token_type::PCTR_MINUS_ASSIGN:
  case token_type::PCTR_BIT_AND_ASSIGN:
  case token_type::PCTR_POWER_ASSIGN:
  case token_type::PCTR_BIT_OR_ASSIGN:
  case token_type::PCTR_SHIFT_LEFT:
  case token_type::PCTR_SHIFT_RIGHT:
  case token_type::PCTR_VARARGS:
  case token_type::PCTR_SHIFT_LEFT_ASSIGN:
  case token_type::PCTR_SHIFT_RIGHT_ASSIGN:
    o << "punctuator " << pctr_to_string(t.type);
    break;
  // handle keywords generically
  case token_type::KWD_AUTO:
  case token_type::KWD_BREAK:
  case token_type::KWD_CASE:
  case token_type::KWD_CHAR:
  case token_type::KWD_CONST:
  case token_type::KWD_CONTINUE:
  case token_type::KWD_DEFAULT:
  case token_type::KWD_DO:
  case token_type::KWD_DOUBLE:
  case token_type::KWD_ELSE:
  case token_type::KWD_ENUM:
  case token_type::KWD_EXTERN:
  case token_type::KWD_FLOAT:
  case token_type::KWD_FOR:
  case token_type::KWD_GOTO:
  case token_type::KWD_IF:
  case token_type::KWD_INLINE:
  case token_type::KWD_INT:
  case token_type::KWD_LONG:
  case token_type::KWD_REGISTER:
  case token_type::KWD_RESTRICT:
  case token_type::KWD_RETURN:
  case token_type::KWD_SHORT:
  case token_type::KWD_SIGNED:
  case token_type::KWD_SIZEOF:
  case token_type::KWD_STATIC:
  case token_type::KWD_STRUCT:
  case token_type::KWD_SWITCH:
  case token_type::KWD_TYPEDEF:
  case token_type::KWD_UNION:
  case token_type::KWD_UNSIGNED:
  case token_type::KWD_VOID:
  case token_type::KWD_VOLATILE:
  case token_type::KWD_WHILE:
  case token_type::KWD__ALIGNAS:
  case token_type::KWD__ALIGNOF:
  case token_type::KWD__ATOMIC:
  case token_type::KWD__BOOL:
  case token_type::KWD__COMPLEX:
  case token_type::KWD__GENERIC:
  case token_type::KWD__IMAGINARY:
  case token_type::KWD__NORETURN:
  case token_type::KWD__STATIC_ASSERT:
  case token_type::KWD__THREAD_LOCAL:
    o << "keyword " << kwd_to_string(t.type);
    break;
  }

  return o;
}

}
