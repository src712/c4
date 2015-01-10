#ifndef C4_PRINT_VISITOR_H
#define C4_PRINT_VISITOR_H

#include <iosfwd>

#include "ast_fwd.h"
#include "ast_visitor.h"

namespace c4 {

struct print_visitor : ast_visitor {
  print_visitor(std::ostream& os) : os_(os) {};

  virtual bool handle(translation_unit*);
  virtual bool handle(declarator*);
  virtual bool handle(parameter_decl*);
  virtual bool handle(decl*);
  virtual bool handle(type_specifier*);
  virtual bool handle(type_name*);
  virtual bool handle(struct_specifier*);

  virtual bool handle(stmt*);
  bool handle_body(comp_stmt*);
  virtual bool handle(comp_stmt*);
  virtual bool handle(break_stmt*);
  virtual bool handle(continue_stmt*);
  virtual bool handle(return_stmt*);
  virtual bool handle(labeled_stmt*);
  virtual bool handle(goto_stmt*);
  virtual bool handle(while_stmt*);
  virtual bool handle(if_stmt*);
  bool handle_impl(if_stmt*);
  virtual bool handle(if_else_stmt*);
  bool handle_impl(if_else_stmt*);
  virtual bool handle(expr_stmt*);

  virtual bool handle(expression*);
  virtual bool handle(primary_expression*);
  virtual bool handle(sizeof_expr*);
  virtual bool handle(sizeof_type*);
  virtual bool handle(unary_operator*); 
  virtual bool handle(binary_operator*);
  virtual bool handle(postfix_operator*);
  virtual bool handle(subscript_operator*);
  virtual bool handle(function_call*);
  virtual bool handle(ternary_expr*);
 private:
  bool stmt_or_comp_printer(stmt*);
  void print_tabs();
  int tabs_ = 0;
  std::ostream& os_;
};

} // c4

#endif /* C4_PRINT_VISITOR_H */
