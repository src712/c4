#ifndef C4_SEMA_VISITOR_H
#define C4_SEMA_VISITOR_H

#include "ast_fwd.h"
#include "ast_visitor.h"
#include "scope.h"

#include <memory>
#include <string>
#include <stack>
#include <map>
#include <vector>

struct type;

namespace c4 {

struct sema_visitor : ast_visitor {
  virtual bool handle(translation_unit*);
  virtual bool handle(declarator*);
  virtual bool handle(parameter_decl*);
  virtual bool handle(decl*);
  virtual bool handle(type_specifier*);
  virtual bool handle(struct_specifier*);

  virtual bool handle(stmt*);
  virtual bool handle(comp_stmt*);
  virtual bool handle(break_stmt*);
  virtual bool handle(continue_stmt*);
  virtual bool handle(return_stmt*);
  virtual bool handle(labeled_stmt*);
  virtual bool handle(goto_stmt*);
  virtual bool handle(while_stmt*);
  virtual bool handle(if_stmt*);
  virtual bool handle(if_else_stmt*);
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
  // context methods
  scope scope_;
  std::stack<decl*> function_scope_;
  int loop_count = 0;

  struct label_compare {
    bool operator() (const std::string* lhs, const std::string* rhs) const {
      return *lhs < *rhs;
    }
  };

  std::map<const std::string*, labeled_stmt*, label_compare> labels_;
  std::vector<goto_stmt*> gotos_;

  std::pair<std::shared_ptr<type>, std::shared_ptr<type>> 
    visit_operands(expression*, expression*, expression*);
  void check_condition(expression* condition, const Pos& pos);
  void check_gotos();
  void handle_functions(decl* d);

  linkage determine_linkage(base_decl* d);
  std::shared_ptr<type> analyze_type(base_decl* d);
  std::shared_ptr<type> analyze_struct(struct_specifier* ss, bool named);
  std::shared_ptr<type> add_struct(struct_specifier* ss, 
				   std::shared_ptr<struct_type> st);
  
};

} // c4

#endif /* C4_SEMA_VISITOR_H */
