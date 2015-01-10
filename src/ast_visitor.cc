#include "ast_visitor.h"
#include "ast.h"
#include "token.h"

namespace c4 {

void ast_visitor::visit(ast_node* n) { n->visit(this); }

// Why the difference between visit(x) and x->visit(this)? Sometimes
// we know the exact type and know that it is not being inherited from
// and we don't need to double dispatch. Then we use the first form,
// the second otherwise.

/////////// DECLARATIONS //////////

void ast_visitor::visit(translation_unit* tu) {
  if(handle(tu)) {
    for(auto& d : tu->decls()) {
      visit(d.get());
    }
  }
}

void ast_visitor::visit(declarator* d) {
  if(handle(d)) {
    if(auto inner = d->get_declarator())
      visit(inner);
  }
}

void ast_visitor::visit(parameter_decl* pd) {
  if(handle(pd)) {
    if(pd->get_type_specifier())
      pd->get_type_specifier()->visit(this);
      
    if(pd->get_declarator())
      visit(pd->get_declarator());
  }
}

void ast_visitor::visit(decl* d) {
  if(handle(d)) {
    d->get_type_specifier()->visit(this);
    if(d->get_declarator())
      d->get_declarator()->visit(this);
    if(d->has_body()) {
      visit(d->get_body()); // no need to double dispatch, can only be a comp_stmt
    }
  }
}

void ast_visitor::visit(type_specifier* ts) {
  handle(ts);
}

void ast_visitor::visit(type_name* tn) {
  if(handle(tn)) {
    tn->get_type_specifier()->visit(this);
    if(auto ad = tn->get_declarator())
     if (ad->pointer() || ad->get_declarator())
       ad->visit(this);
  }
}

void ast_visitor::visit(struct_specifier* s) {
  if(handle(s)) {
    for(auto& x : s->decls()) {
      visit(x.get());
    }
  }
}

////////// STATEMENTS //////////

void ast_visitor::visit(stmt* s) {
  s->visit(this);
}

void ast_visitor::visit(comp_stmt* cs) {
  if(handle(cs)) {
    for(auto& x : cs->sub_stmts()) {
      x->visit(this);
    }
  }
}

void ast_visitor::visit(break_stmt* bs) {
  handle(bs);
}

void ast_visitor::visit(continue_stmt* cs) {
  handle(cs);
}

void ast_visitor::visit(return_stmt* rs) {
  if(handle(rs)) {
    if(auto expr = rs->expr())
      expr->visit(this);
  }
}

void ast_visitor::visit(labeled_stmt* ls) {
  if(handle(ls))
    ls->get_stmt()->visit(this);
}

void ast_visitor::visit(goto_stmt* gs) {
  handle(gs);
}

void ast_visitor::visit(while_stmt* ws) {
  if(handle(ws)) {
    ws->condition()->visit(this);
    ws->body()->visit(this);
  }
}

void ast_visitor::visit(if_stmt* is) {
  if(handle(is)) {
    is->condition()->visit(this);
    is->body()->visit(this);
  }
}

void ast_visitor::visit(if_else_stmt* ies) {
  if(handle(ies)) {
    ies->condition()->visit(this);
    ies->if_body()->visit(this);
    ies->else_body()->visit(this);
  }
}

void ast_visitor::visit(expr_stmt* e) {
  if(handle(e))  {
    if(auto expr = e->expr())
      expr->visit(this);
  }
}

////////// EXPRESSIONS //////////

void ast_visitor::visit(expression* e) {
  handle(e);
}

void ast_visitor::visit(primary_expression* pe) {
  handle(pe);
}

void ast_visitor::visit(sizeof_expr* s) {
  if(handle(s)) {
    s->expr()->visit(this);
  }
}

void ast_visitor::visit(sizeof_type* s) {
  if(handle(s)) {
    s->get_type_name()->visit(this);
  }
}

void ast_visitor::visit(unary_operator* o) {
  if(handle(o)) {
    o->operand()->visit(this);
  }
}

void ast_visitor::visit(binary_operator* o) {
  if(handle(o)) {
    o->left()->visit(this);
    o->right()->visit(this);
  }
}

void ast_visitor::visit(postfix_operator* o) {
  if(handle(o)) {
    o->left()->visit(this);
    o->right()->visit(this);
  }
}

void ast_visitor::visit(subscript_operator* o) {
  if(handle(o)) {
    o->left()->visit(this);
    o->right()->visit(this);
  }
}

void ast_visitor::visit(function_call* fc) {
  if(handle(fc)) {
    for(auto& x : fc->params()) {
      x->visit(this);
    }
  }
}

void ast_visitor::visit(ternary_expr* te) {
  if(handle(te)) {
    te->test()->visit(this);
    te->true_expr()->visit(this);
    te->false_expr()->visit(this);
  }
}

}
