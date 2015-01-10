#ifndef C4_STMT_PARSER_H
#define C4_STMT_PARSER_H

#include "parser_base.h"

namespace c4 {

struct stmt;
struct while_stmt;
struct labeled_stmt;
struct expr_stmt;
struct break_stmt;
struct continue_stmt;
struct goto_stmt;
struct return_stmt;
struct if_stmt;
struct if_else_stmt;
struct comp_stmt;

struct stmt_parser : parser_base {
  using parser_base::parser_base;
  stmt* operator()();

private:
  comp_stmt* match_compound();
  labeled_stmt* match_labeled();
  while_stmt* match_while();
  stmt* match_if();
  stmt* match_jump();
  expr_stmt* match_expression_statement();
};

} // c4

#endif /* C4_STMT_PARSER_H */
