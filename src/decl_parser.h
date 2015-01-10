#ifndef C4_DECL_PARSER_H
#define C4_DECL_PARSER_H

#include "parser_base.h"

namespace c4 {

struct decl;
struct type_specifier;
struct declarator;
struct pointer;
struct type_specifier;
struct struct_specifier;

struct decl_parser : parser_base
{
  using parser_base::parser_base;
  decl* operator()();
  type_specifier* match_type_specifier();
  enum declarator_kind { NON_ABSTRACT, ABSTRACT, BOTH};
  declarator* match_declarator(declarator_kind kind = NON_ABSTRACT);
private:
  struct_specifier* match_struct();
  void match_struct_declaration_list(struct_specifier*);
  void match_parameter_type_list(declarator* d);
  void match_parameter_type(declarator* d);
};

inline bool is_type_specifier(const token& t) {
  return t.type == token_type::KWD_VOID
    || t.type == token_type::KWD_CHAR
    || t.type == token_type::KWD_INT
    || t.type == token_type::KWD_STRUCT
    || t.type == token_type::KWD_UNION;
}

} // c4
#endif /* C4_DECL_PARSER_H */
