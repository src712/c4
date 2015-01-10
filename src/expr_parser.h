#ifndef C4_EXPR_PARSER_H
#define C4_EXPR_PARSER_H

#include "parser_base.h"

namespace c4 {

struct expression;
struct primary_expression;
struct error_expr;
struct operand;
struct sizeof_expr;
struct sizeof_type;

struct expr_parser : parser_base
{
  using parser_base::parser_base;
  expression* operator()();
 private:
  expression* match_operand();
  expression* match_subexpression(expression* left, int min_prec);
  expression* match_unary_expression();
  expression* match_postfix_expression();
  expression* match_primary_expression();
};

inline bool is_unary_operator(const token& t) {
  return t.type == token_type::PCTR_BIT_AND
    || t.type == token_type::PCTR_MINUS
    || t.type == token_type::PCTR_STAR
    || t.type == token_type::PCTR_BANG;
}

inline bool is_postfix_operator(const token& t) {
  return t.type == token_type::PCTR_LBRACKET
    || t.type == token_type::PCTR_LPAREN
    || t.type == token_type::PCTR_DOT
    || t.type == token_type::PCTR_DEREF;
}

inline bool is_ternary(const token& t) {
  return t.type == token_type::PCTR_QUESTIONMARK;
}

} // c4
#endif /* C4_EXPR_PARSER_H */
