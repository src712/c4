#ifndef C4_AST_VISITOR_H
#define C4_AST_VISITOR_H

#include "ast_fwd.h"

namespace c4 {

struct ast_visitor {
  void visit(ast_node*);

  void visit(translation_unit*);
  void visit(declarator*); 
  void visit(parameter_decl*);
  void visit(decl*);
  void visit(type_specifier*);
  void visit(type_name*);
  void visit(struct_specifier*);

  void visit(stmt*);
  void visit(comp_stmt*);
  void visit(break_stmt*);
  void visit(continue_stmt*);
  void visit(return_stmt*);
  void visit(labeled_stmt*);
  void visit(goto_stmt*);
  void visit(while_stmt*);
  void visit(if_stmt*);
  void visit(if_else_stmt*);
  void visit(expr_stmt*);

  void visit(expression*);
  void visit(primary_expression*);
  void visit(sizeof_expr*);
  void visit(sizeof_type*);
  void visit(unary_operator*);
  void visit(binary_operator*);
  void visit(postfix_operator*);
  void visit(subscript_operator*);
  void visit(function_call*);
  void visit(ternary_expr*);


  virtual bool handle(translation_unit*) { return true; }
  virtual bool handle(declarator*) { return true; }
  virtual bool handle(parameter_decl*) { return true; }
  virtual bool handle(decl*) { return true; }
  virtual bool handle(type_specifier*) { return true; }
  virtual bool handle(type_name*) { return true; }
  virtual bool handle(struct_specifier*) { return true; }

  virtual bool handle(stmt*) { return true; }
  virtual bool handle(comp_stmt*) { return true; }
  virtual bool handle(break_stmt*) { return true; }
  virtual bool handle(continue_stmt*) { return true; }
  virtual bool handle(return_stmt*) { return true; }
  virtual bool handle(labeled_stmt*) { return true; }
  virtual bool handle(goto_stmt*) { return true; }
  virtual bool handle(while_stmt*) { return true; }
  virtual bool handle(if_stmt*) { return true; }
  virtual bool handle(if_else_stmt*) { return true; }
  virtual bool handle(expr_stmt*) { return true; }

  virtual bool handle(expression*) { return true; }
  virtual bool handle(primary_expression*) { return true; }
  virtual bool handle(sizeof_expr*) { return true; }
  virtual bool handle(sizeof_type*) { return true; }
  virtual bool handle(unary_operator*) { return true; } 
  virtual bool handle(binary_operator*) { return true; }
  virtual bool handle(postfix_operator*) { return true; }
  virtual bool handle(subscript_operator*) { return true; }
  virtual bool handle(function_call*) { return true; }
  virtual bool handle(ternary_expr*) { return true; }

  virtual ~ast_visitor() {}
};

} // c4

#endif /* C4_AST_VISITOR_H */
