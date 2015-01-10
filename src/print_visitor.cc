#include "print_visitor.h"
#include "ast.h"

#include <ostream>

namespace c4 {

void print_visitor::print_tabs() {
  os_ << '\n';
  for(int i = 0; i < tabs_; i++)
    os_ << "\t";
}

bool print_visitor::stmt_or_comp_printer(stmt* s) {
  comp_stmt* cs = dynamic_cast<comp_stmt*> (s);
  if(!cs) {
    ++tabs_;
    visit(s);
    --tabs_;
  } else {
    os_ << " {";
    handle_body(cs);
  }
  return cs;
}

/////////// DECLARATIONS //////////

bool print_visitor::handle(translation_unit* tu) {
  for(auto it = begin(tu->decls()); it != end(tu->decls()); ++it) {
    (*it)->visit(this);
    if(it != --end(tu->decls()))
      os_ << '\n';
    os_ << '\n';
  }
  return false;
}

bool print_visitor::handle(declarator* d) {
  bool p = d->pointer();
  declarator* inner = d->get_declarator();
  std::string identifier = d->get_identifier();
  bool parens = p || (d->parameter_decls().size() != 0 );

  if(parens) 
    os_ << '(';
  if(p)
    os_ << '*';  
  os_ << identifier;
  if(inner)
    handle(inner);

  if(d->parameter_decls().size() != 0) {
    os_ << '(';
    auto endit = d->parameter_decls().end();
    for(auto it = d->parameter_decls().begin(); it != endit; ++it) {
      handle(it->get());
      if(it != std::prev(endit))
	os_ << ", ";
    }
    os_ << ')';
  }

  if(parens)
    os_ << ')';

  return false;
}

bool print_visitor::handle(parameter_decl* pd) {
  if(pd->get_type_specifier()) {
    pd->get_type_specifier()->visit(this);
  }

  if(pd->get_declarator()) {
    os_ << ' ';
    visit(pd->get_declarator());
  }

  return false;
}

bool print_visitor::handle(decl* d) {
  if(tabs_ != 0)
    print_tabs();

  d->get_type_specifier()->visit(this);
  if(d->get_declarator()) {
    os_ << ' ';
    visit(d->get_declarator());
  }

  if(d->has_body()) {
    os_ << '\n' << '{';
    handle_body(d->get_body());
  } else
    os_ << ';';
  
  return false;
}

bool print_visitor::handle(type_specifier* ts) {
  os_ << token_to_string(ts->token);
  return true;
}

bool print_visitor::handle(type_name* tn) {
   tn->get_type_specifier()->visit(this);
   if(auto ad = tn->get_declarator()) {
      if (ad->pointer() || ad->get_declarator()) {
	os_ << ' ';
	ad->visit(this);
      }
   }
    return false;
}

bool print_visitor::handle(struct_specifier* s) {
  os_ << "struct " << s->get_tag();
  if(s->has_decls()) {
    print_tabs();
    os_ << '{';
    ++tabs_;
  }

  for(auto& x : s->decls()) {
    visit(x.get());
  }

  if(s->has_decls()) {
    --tabs_;
    print_tabs();
    os_ << '}';
  }

  return false;
}

////////// STATEMENTS //////////

bool print_visitor::handle(stmt*) {
  return true;
}

bool print_visitor::handle(comp_stmt* cs) {
  print_tabs();
  os_ << '{';
  return handle_body(cs);
}

bool print_visitor::handle_body(comp_stmt* cs) {
  ++tabs_;
  for(auto& x : cs->sub_stmts()) {
    x->visit(this);
  }
  --tabs_;
  print_tabs();
  os_ << '}';
  return false;
}

bool print_visitor::handle(break_stmt*) {
  print_tabs();
  os_ << "break;";
  return true;
}

bool print_visitor::handle(continue_stmt*) {
  print_tabs();
  os_ << "continue;";
  return true;
}

bool print_visitor::handle(return_stmt* rs) {
  print_tabs();
  os_ << "return";
  if(auto expr = rs->expr()) {
    os_ << ' ';
    expr->visit(this);
  }
  os_ << ';';
  return false;
}

bool print_visitor::handle(labeled_stmt* ls) {
  os_ << '\n' << ls->get_label() << ':';
  return true;
}

bool print_visitor::handle(goto_stmt* gs) {
  print_tabs();
  os_ << "goto " << gs->get_label() << ';';
  return true;
}

bool print_visitor::handle(while_stmt* ws) {
  print_tabs();
  os_ << "while (";
  ws->condition()->visit(this);
  os_ << ')';
  stmt_or_comp_printer(ws->body());
  return false;
}

bool print_visitor::handle(if_stmt* is) {
  print_tabs();
  return handle_impl(is);
}

bool print_visitor::handle_impl(if_stmt* is) {
  os_ << "if (";
  is->condition()->visit(this);
  os_ << ')';
  stmt_or_comp_printer(is->body());
  return false;
}

bool print_visitor::handle(if_else_stmt* ies) {
  print_tabs();
  return handle_impl(ies);
}

bool print_visitor::handle_impl(if_else_stmt* ies) {
  os_ << "if (";
  ies->condition()->visit(this);
  os_ << ')';
  bool fcomp = stmt_or_comp_printer(ies->if_body());
  if(fcomp) {
    os_ << " else";
  } else {
    print_tabs();
    os_ << "else";
  }

  if(if_stmt* eis = dynamic_cast<if_stmt*>(ies->else_body())) {
    os_ << ' ';
    handle_impl(eis);
    return false;
  } else if(if_else_stmt* eies =
	    dynamic_cast<if_else_stmt*> (ies->else_body())) {
    os_ << ' ';
    handle_impl(eies);
    return false;
  }
  stmt_or_comp_printer(ies->else_body());
  return false;
}

bool print_visitor::handle(expr_stmt* e) {
  print_tabs();
  if(auto expr = e->expr())
    expr->visit(this);
  os_ << ';';
  return false;
}

////////// EXPRESSIONS //////////

bool print_visitor::handle(expression*) {
  return true;
}

bool print_visitor::handle(primary_expression* pe) {
  os_ << (pe->value()).data.str();
  return true;
}

bool print_visitor::handle(sizeof_expr* se) {
  os_ << "(sizeof ";
  se->expr()->visit(this);
  os_ << ')';
  return false;
}

bool print_visitor::handle(sizeof_type* st) {
  os_ << "(sizeof(";
  st->get_type_name()->visit(this);
  os_ << "))";
  return false;
}

bool print_visitor::handle(unary_operator* uo) {
  os_ << '(' << token_to_string(uo->op());
  uo->operand()->visit(this);
  os_ << ')';
  return false;
}

bool print_visitor::handle(binary_operator* bo) {
  os_ << '(';
  bo->left()->visit(this);
  os_ << ' ' << token_to_string(bo->op()) << ' ';
  bo->right()->visit(this);
  os_ << ')';
  return false;
}

bool print_visitor::handle(postfix_operator* po) {
  os_ << '(';
  po->left()->visit(this);
  os_ << token_to_string(po->op());
  po->right()->visit(this);
  os_ << ')';
  return false;
}

bool print_visitor::handle(subscript_operator* ao) {
  os_ << '(';
  ao->left()->visit(this);
  os_ << '[';
  ao->right()->visit(this);
  os_ << "])";
  return false;
}

bool print_visitor::handle(function_call* fc) {
  os_ << '(';
  fc->get_name()->visit(this);
  os_ << '(';
  for(auto it = begin(fc->params()); it != end(fc->params()); ++it) {
    (*it)->visit(this);
    if(it != --end(fc->params()))
      os_ << ", ";
  }
  os_ << "))";
  return false;
}

bool print_visitor::handle(ternary_expr* te) {
  os_ << '(';
  te->test()->visit(this);
  os_ << " ? ";
  te->true_expr()->visit(this);
  os_ << " : ";
  te->false_expr()->visit(this);
  os_ << ')';
  return false;
}

}
