#include "sema_visitor.h"
#include "diagnostic.h"
#include "ast.h"
#include "type.h"
#include "pos.h"
#include <cassert>
#include <string>

namespace {
bool handle_sizeof(c4::expression*, const std::shared_ptr<c4::type>& s_type);
bool both_arith(c4::binary_operator* bo, 
		std::shared_ptr<c4::type> ltype, 
		std::shared_ptr<c4::type> rtype);
bool ptr_arith(c4::binary_operator* bo, 
	       std::shared_ptr<c4::type> ltype, 
	       std::shared_ptr<c4::type> rtype);
bool ptr_null(c4::binary_operator* bo, 
	      std::shared_ptr<c4::type> ltype, 
	      std::shared_ptr<c4::type> rtype);
bool equal_comp_ptrs(c4::binary_operator* bo, 
		     std::shared_ptr<c4::type> ltype, 
		     std::shared_ptr<c4::type> rtype);
bool equal_obj_ptrs(c4::binary_operator* bo, 
		    std::shared_ptr<c4::type> ltype, 
		    std::shared_ptr<c4::type> rtype);
bool ptr_obj_ptr_void(c4::binary_operator* bo, 
		      std::shared_ptr<c4::type> ltype, 
		      std::shared_ptr<c4::type> rtype);
bool both_scalar(c4::binary_operator* bo, 
		 std::shared_ptr<c4::type> ltype, 
		 std::shared_ptr<c4::type> rtype);
bool assign_compatible(const std::shared_ptr<c4::type>& ltype,
		       const std::shared_ptr<c4::type>& rtype);
bool ternary_lcompatible(const std::shared_ptr<c4::type>& ltype,
		       const std::shared_ptr<c4::type>& rtype);
bool ternary_rcompatible(const std::shared_ptr<c4::type>& ltype,
		       const std::shared_ptr<c4::type>& rtype);
bool is_lvalue(c4::expression* left);
}

namespace c4 {

bool sema_visitor::handle(translation_unit*) {
  return true;
}
bool sema_visitor::handle(declarator*) {
  return true;
}
bool sema_visitor::handle(parameter_decl*) {
  return true;
}
bool sema_visitor::handle(decl* d) {
   if(!d->has_type())
     analyze_type(d);

   d->set_linkage(determine_linkage(d));

  if(!d->get_name().empty() && !scope_.put(d)) {
    // The identifier already exists in the current scope.
    // Are the types the same?
    auto other = scope_.get(d->get_name());

    if(*d->get_type() != *other->get_type())
      errorf(d->position(), "'%s' redeclared with a different type",
	     d->get_name().c_str());
    if(d->get_linkage() != linkage::EXTERNAL)
      errorf(d->position(), "'%s' redeclared",
	     d->get_name().c_str());

    if(auto otherf = dynamic_cast<decl*>(other)) {
      if(otherf->has_body() && d->has_body())
        errorf(d->position(), "'%s' redefined",
               d->get_name().c_str());
    }
  } else if(!d->get_type()->is_complete() &&
	    !d->get_name().empty() &&
	    !d->get_type()->is_function()) {
    errorf(d->position(), "storage size of '%s' isn't known",
	   d->get_name().c_str());
    d->set_type(error_type::get_error());
  }

  if(d->get_type()->is_function())
    handle_functions(d);

  return false;
}

void sema_visitor::handle_functions(decl* d) {
  scope_.enter_scope(); // extra scope for the function arguments
  if(d->has_body()) {
    scope_.ignore_next();
    function_scope_.push(d);
  }

  // insert all function decls into the current scope
  auto ftype = as_function_type(d->get_type());
  assert(ftype); int pos = 0;
  for(auto& x : d->parameter_decls()) {
    ++pos;
    if(x->get_name().empty()) {
      if(x->get_type()->is_void()) {
	if(d->parameter_decls().size() != 1)
	  errorf(x->position(), "'void' must be the only parameter");
	continue;
      } else if(d->has_body()) {
	  errorf(x->position(), "parameter name omitted");
      }
    } else {
      if(!scope_.put(x.get()))
	errorf(x->position(), "'%s' redeclared", x->get_name().c_str());
      x->set_linkage(linkage::NONE);
    }
    if(!x->get_type()->is_complete() && d->has_body())
      errorf(x->position(), "parameter %s has incomplete type",
             std::to_string(pos).c_str());
  }

  if(d->has_body()) {
    d->get_body()->visit(this);
    check_gotos();
    assert(!function_scope_.empty());
    function_scope_.pop();
  }
  scope_.leave_scope();
}
 
void sema_visitor::check_gotos() {
  for(auto& gs : gotos_) {
    auto it = labels_.find(&(gs->get_label()));
    if(it == labels_.end())
      errorf(gs->position(), "label '%s' used but not defined",
	     gs->get_label().c_str());
    else
      gs->set_label(it->second);
  }
}
bool sema_visitor::handle(type_specifier*) {
  return true;    
}
bool sema_visitor::handle(struct_specifier*) {
  return false;
}


bool sema_visitor::handle(stmt*) {
  return true;
}

bool sema_visitor::handle(comp_stmt* cs) {
  scope_.enter_scope();
  for(auto& x : cs->sub_stmts()) {
    x->visit(this);
  }
  scope_.leave_scope();
  return false;
}

bool sema_visitor::handle(break_stmt* bs) {
  if(!loop_count)
    errorf(bs->position(), "break statement not within loop");
  return true;
}

bool sema_visitor::handle(continue_stmt* cs) {
  if(!loop_count)
    errorf(cs->position(), "continue statement not within loop");
  return true;
}

bool sema_visitor::handle(return_stmt* rs) {
  assert(!function_scope_.empty());
  auto ftype = as_function_type(function_scope_.top()->get_type());
  rs->exp_rtype() = ftype->return_type();
  if(auto rexpr = rs->expr()) {
    rexpr->visit(this);
    auto rtype = rexpr->e_type();
    if(rtype->is_error())
      return false;
    else if(rs->exp_rtype()->is_void())
     errorf(rs->position(),
	   "'return' with an expression in function returning void");
    else if(!assign_compatible(rs->exp_rtype(), rtype))
      errorf(rs->position(),
	     "incompatible types when returning");
  } else if(!(rs->exp_rtype()->is_void()))
    errorf(rs->position(),
	   "'return' with no value in function returning non-void");
  return false;
}

bool sema_visitor::handle(labeled_stmt* ls) {
  if(!labels_.insert(std::make_pair(&(ls->get_label()), ls)).second)
    errorf(ls->position(), "duplicate label '%s'", ls->get_label().c_str());
  return true;
}

bool sema_visitor::handle(goto_stmt* gs) {
  gotos_.emplace_back(gs);
  return true;
}

void sema_visitor::check_condition(expression* condition, const Pos& pos) {
  condition->visit(this);
  auto cond = condition->e_type();
  if(!(cond->is_scalar()) && !(cond->is_error()))
    errorf(pos, "used non-scalar type where scalar is required");
}

bool sema_visitor::handle(while_stmt* ws) {
  ++loop_count;
  scope_.enter_scope();
  check_condition(ws->condition(), ws->position());
  ws->body()->visit(this);
  scope_.leave_scope();
  --loop_count;
  return false;
}

bool sema_visitor::handle(if_stmt* is) {
  check_condition(is->condition(), is->position());
  is->body()->visit(this);
  return false;
}

bool sema_visitor::handle(if_else_stmt* ies) {
  check_condition(ies->condition(), ies->position());
  ies->if_body()->visit(this);
  ies->else_body()->visit(this);
  return false;
}

bool sema_visitor::handle(expr_stmt*) {
  return true;
}



bool sema_visitor::handle(expression*) {
  return true;
}

bool sema_visitor::handle(primary_expression* pe) {
  switch(pe->value().type) {
  case token_type::INTEGER_CONSTANT: {
    if(pe->value().data == "0")
      pe->e_type() = arithmetic_type::get_zero();
    else
      pe->e_type() = arithmetic_type::get_int();
    break;
  }
  case token_type::CHARACTER_CONSTANT: {
    pe->e_type() = arithmetic_type::get_int();
    break;
  }
  case token_type::STRING_LITERAL: {
    pe->e_type().reset(new pointer_type{arithmetic_type::get_char()});
    break;
  }
  default: { // identifier
    auto iden_decl = scope_.get(pe->value().data);
    if(!iden_decl) {
      errorf(pe->position(), "'%s' undeclared",
	     pe->value().data.str().c_str());
      pe->e_type() = error_type::get_error();
    } else {
      assert(iden_decl->has_type());
      pe->e_type() = iden_decl->get_type();
    }
  }
  }
  return false;
}

bool sema_visitor::handle(sizeof_expr* s) {
  s->expr()->visit(this);
  return handle_sizeof(s, s->expr()->e_type());
}

bool sema_visitor::handle(sizeof_type* s) {
  return handle_sizeof(s, analyze_type(s->get_type_name()));
}

bool sema_visitor::handle(unary_operator* uo) {
  uo->operand()->visit(this);
  auto& otype = uo->operand()->e_type();

  if(otype->is_error()) {
    uo->e_type() = otype;
    return false;
  }

  switch(uo->op()) {
  case token_type::PCTR_BANG:
    if(otype->is_scalar())
      uo->e_type() = arithmetic_type::get_int();
    break;
  case token_type::PCTR_MINUS:
    if(otype->is_arithmetic())
      uo->e_type() = arithmetic_type::get_int();
    break;
  case token_type::PCTR_STAR:
    if(auto ptype = as_pointer_type(otype))
      uo->e_type() = ptype->underlying();
    break;
  default: // PCTR_BIT_AND
    assert(uo->op() == token_type::PCTR_BIT_AND);
    if(otype->is_function() || is_lvalue(uo->operand()))
      uo->e_type().reset(new pointer_type{otype});
  }

  if(!(uo->e_type()))  {
    uo->e_type() = error_type::get_error();
    errorf(uo->position(), "invalid type argument to unary '%s'",
	   token_to_string(uo->op()));
  }
  return false;
}

std::pair<std::shared_ptr<type>, std::shared_ptr<type>> 
  sema_visitor::visit_operands(expression* e, 
			       expression* left, expression* right ) {
  left->visit(this);
  auto ltype = left->e_type();
  right->visit(this);
  auto rtype = right->e_type();

  // ensure that subexpressions do not contain errors
  if(ltype->is_error() || rtype->is_error())
    e->e_type() = error_type::get_error();

  return std::make_pair(ltype, rtype);
}

bool sema_visitor::handle(binary_operator* bo) {
  std::shared_ptr<type> ltype, rtype;
  std::tie(ltype, rtype) = visit_operands(bo, bo->left(), bo->right());
  if(bo->e_type()) return false;

  switch(bo->op()) {
  case token_type::PCTR_PLUS: {
    ptr_arith(bo, ltype, rtype) ||
      ptr_arith(bo, rtype, ltype) ||
      both_arith(bo, ltype, rtype);
    break;
  }
  case token_type::PCTR_STAR: {
    both_arith(bo, ltype, rtype);
    break;
  }
  case token_type::PCTR_MINUS: {
    both_arith(bo, ltype, rtype) ||
      ptr_arith(bo, ltype, rtype) ||
      equal_comp_ptrs(bo, ltype, rtype);
    break;
  }
  case token_type::PCTR_EQUAL:
  case token_type::PCTR_NOT_EQUAL: {
    ptr_obj_ptr_void(bo, ltype, rtype) ||
      ptr_null(bo, ltype, rtype) ||
      both_arith(bo, ltype, rtype) ||
      equal_obj_ptrs(bo, ltype, rtype);
    break;
  }
  case token_type::PCTR_LESS: {
    both_arith(bo, ltype, rtype) ||
      equal_obj_ptrs(bo, ltype, rtype);
    break;
  }
  case token_type::PCTR_AND:
  case token_type::PCTR_OR: {
    both_scalar(bo, ltype, rtype);
    break;
  }
  default: { // case token_type::PCTR_ASSIGN:
    if(ltype->is_complete() && is_lvalue(bo->left())) {
      if(assign_compatible(ltype, rtype)) {
	bo->e_type() = ltype;
      } else { // incompatible types
	bo->e_type() = error_type::get_error();
	errorf(bo->position(), "incompatible types for assignment");
      }
    } else { // left operand isn't an lvalue
      bo->e_type() = error_type::get_error();
      errorf(bo->position(),
	     "modifiable lvalue required as left operand of assignment");
    }
  }
  }

  if(!(bo->e_type()))  {
    bo->e_type() = error_type::get_error();
    errorf(bo->position(), "invalid operands to binary '%s'",
	   token_to_string(bo->op()));
  }
  return false;
}



bool sema_visitor::handle(postfix_operator* po) {
   po->left()->visit(this);
   auto ltype = po->left()->e_type();
   if(ltype->is_error()) {
     po->e_type() = ltype;
     return false;
   } 

  if(po->op() == token_type::PCTR_DEREF) {
    if(!(ltype->is_pointer())) {
      po->e_type() = error_type::get_error();
      errorf(po->position(),
	     "invalid type argument of '->', expected pointer to struct");
      return false;
    }
    ltype = as_pointer_type(ltype)->underlying();
  }

  if(auto lstruct = as_struct_type(ltype)) {
    auto right = dynamic_cast<primary_expression*> (po->right());
    auto& iden = right->value().data;
    if(auto mtype = lstruct->lookup(iden)) {
      po->e_type() = mtype;
    } else {
      po->e_type() = error_type::get_error();
      errorf(po->position(), "no member '%s' in given struct", iden.str().c_str());
    }
  } else {
    po->e_type() = error_type::get_error();
    if(po->op() == token_type::PCTR_DEREF)
      errorf(po->position(),
	     "invalid type argument of '->', expected pointer to struct");
    else
      errorf(po->position(),
	     "invalid type argument of '.', expected struct");
  }

  return false;
}

bool sema_visitor::handle(subscript_operator* so) {
  std::shared_ptr<type> ltype, rtype;
  std::tie(ltype, rtype) = visit_operands(so, so->left(), so->right());
  if(so->e_type()) return false;

  if(ltype->is_arithmetic() && rtype->pointer_to_complete()) {
    so->e_type() = as_pointer_type(rtype)->underlying();
  } else if(rtype->is_arithmetic() && ltype->pointer_to_complete()) {
    so->e_type() = as_pointer_type(ltype)->underlying();
  } else {
    so->e_type() = error_type::get_error();
    errorf(so->position(), "invalid operands for array subscripting");
  }

  return false;
}

bool sema_visitor::handle(function_call* fc) {
  fc->get_name()->visit(this);
  auto& name = fc->get_name()->e_type();
  if(name->is_error()) {
    fc->e_type() = name;
    return false;
  }

  std::shared_ptr<function_type> func_name = as_function_type(name);
  if(!func_name)
    if(std::shared_ptr<pointer_type> ptr_func = as_pointer_type(name))
      func_name = as_function_type(ptr_func->underlying());

  if(func_name) {
    auto& params = fc->params();
    auto& args = func_name->arguments();

    // check that number of arguments match
    if(params.size() != func_name->argument_size()) {
      errorf(fc->position(),
	     "passed %s argument(s) to function expecting %s",
             std::to_string(params.size()).c_str(), 
	     std::to_string(func_name->argument_size()).c_str());
      fc->e_type() = error_type::get_error();
      return false;
    }

    bool error = false;
    auto pit = begin(params);
    auto ait = begin(args);
    for(int i = 0; pit != end(params); ++pit, ++ait, ++i) {
      (*pit)->visit(this);
      auto& param_type = (*pit)->e_type();
      if(param_type->is_error()) {
	error = true;
      } else if(!assign_compatible(*ait, param_type)) {
	error = true;
	errorf(fc->position(),
	       "argument at position %s does not have expected type", 
	       std::to_string(i).c_str());
      }
    }

    // set type to return type
    fc->e_type() = error ? error_type::get_error() : func_name->return_type();

  } else {
    fc->e_type() = error_type::get_error();
    errorf(fc->position(),
	   "called object that is not a function or pointer to a function");
  }
  return false;
}

bool sema_visitor::handle(ternary_expr* te) {
  te->test()->visit(this);
  auto& cond = te->test()->e_type();
  std::shared_ptr<type> ltype, rtype;
  std::tie(ltype, rtype) = visit_operands(te, te->true_expr(), te->false_expr());
  bool error = false;

  if(!(cond->is_scalar())) {
    error = true;
    if(!(cond->is_error()))
      errorf(te->position(),
 	     "used non-scalar type where scalar is required");
  }
  
  if(te->e_type()) return false;

  if(ltype->is_arithmetic() && rtype->is_arithmetic()) {
    te->e_type() = arithmetic_type::get_int(); // set type to int
  } else if(ternary_lcompatible(ltype, rtype)) {
    te->e_type() = ltype; // set type to ltype
  } else if(ternary_rcompatible(ltype, rtype)) {
    te->e_type() = rtype; // set type to rtype
  } else {
    error = true;
    errorf(te->position(), "type mismatch in conditional expression");
  }

  if(error)
    te->e_type() = error_type::get_error();

  return false;
}

linkage sema_visitor::determine_linkage(base_decl* d) {
  if(dynamic_cast<parameter_decl*>(d)) {
    return linkage::NONE;
  }
  if(function_scope_.empty()) {
    return linkage::EXTERNAL;
  }
  return linkage::INTERNAL;
}

std::shared_ptr<type> sema_visitor::analyze_type(base_decl* d) {
  auto ts = d->get_type_specifier();
  std::shared_ptr<type> from_ts;

  if(auto ss = dynamic_cast<struct_specifier*>(ts)) {
    bool named = !d->get_name().empty() || dynamic_cast<type_name*>(d);
    from_ts = analyze_struct(ss, named);
  } else {
    switch(ts->token) {
    case token_type::KWD_INT: from_ts = arithmetic_type::get_int(); break;
    case token_type::KWD_CHAR: from_ts = arithmetic_type::get_char(); break;
    case token_type::KWD_VOID: from_ts = void_type::get_void(); break;
    default: from_ts = error_type::get_error(); break;
    };
  }

  bool is_function_decl = false;
  auto dl = d->get_declarator();
  if(!dl) { // a very reasonable case
    d->set_type(from_ts);
    return from_ts;
  }

  do {
    if(dl->pointer())
      from_ts = std::make_shared<pointer_type>(from_ts);

    if(!dl->parameter_decls().empty()) {
      auto ft = std::make_shared<function_type>(from_ts);
      for(auto& x : dl->parameter_decls()) {
        auto type = analyze_type(x.get());
        ft->add_argument_type(type);
      }
      from_ts = ft;
    }

    is_function_decl = !dl->parameter_decls().empty();
    if(is_function_decl && !as_function_type(from_ts))
      from_ts = std::make_shared<function_type>(from_ts);
  } while((dl = dl->get_declarator()));

  d->set_type(from_ts);
  return from_ts;
}

std::shared_ptr<type> sema_visitor::analyze_struct(struct_specifier* ss, 
						   bool named) {
  if(!ss->decls().empty()) { // complete type
    if(auto other = scope_.get_tag_in_current_scope(ss->get_tag())) {
      if(other->is_complete()) { // redefined struct
	errorf(ss->position(), "'struct %s' redefined",
	     ss->get_tag().c_str());
	return error_type::get_error();
      } else { // forward decl in scope
	return add_struct(ss, other);
      }
    } else { // not in current scope
      auto st = std::make_shared<struct_type>(ss->get_tag());
      return add_struct(ss, st);
    }
  } else { // incomplete
    std::shared_ptr<struct_type> other = named 
      ? scope_.get_tag(ss->get_tag()) 
      : scope_.get_tag_in_current_scope(ss->get_tag());
    
    if(other)
      return other;
   
    auto st = std::make_shared<struct_type>(ss->get_tag());
    scope_.put(st);
    return st;    
   
  }
}

std::shared_ptr<type> sema_visitor::add_struct(struct_specifier* ss,
					      std::shared_ptr<struct_type> st) {
  for(auto& x : ss->decls()) {
    st->add_member(std::make_pair(x->get_name(), analyze_type(x.get())));
  }
  
  scope_.put(st);
  function_scope_.push(nullptr);
  scope_.enter_scope();
  for(auto& d : ss->decls()) {
    d->visit(this);
  }
  scope_.leave_scope();
  function_scope_.pop();
  
  return st;
}

}


namespace {

bool handle_sizeof(c4::expression* s, const std::shared_ptr<c4::type>& s_type) {
  if(s_type->is_error())
    s->e_type() = s_type;
  else if(s_type->is_function())
    errorf(s->position(),
	   "invalid application of 'sizeof' to a function type");
  else if(!s_type->is_complete())
    errorf(s->position(),
	   "invalid application of 'sizeof' to an incomplete type");
  else
    s->e_type() = c4::arithmetic_type::get_int();

  if(!(s->e_type()))
    s->e_type() = c4::error_type::get_error();
  return false;
}

bool ptr_obj_ptr_void(c4::binary_operator* bo, 
		      std::shared_ptr<c4::type> ltype, 
		      std::shared_ptr<c4::type> rtype) {
  bool cond = (ltype->is_pointer() && rtype->is_pointer() &&
	       ((ltype->pointer_to_object() && 
		 as_pointer_type(rtype)->underlying()->is_void()) ||
		(rtype->pointer_to_object() && 
		 as_pointer_type(ltype)->underlying()->is_void())));
  if(cond)
    bo->e_type() = c4::arithmetic_type::get_int(); 
  return cond;
}

bool both_arith(c4::binary_operator* bo, 
		std::shared_ptr<c4::type> ltype, 
		std::shared_ptr<c4::type> rtype) {
  bool cond = (ltype->is_arithmetic() && rtype->is_arithmetic());
  if(cond)
    bo->e_type() = c4::arithmetic_type::get_int();
  return cond;
}

bool both_scalar(c4::binary_operator* bo, 
		 std::shared_ptr<c4::type> ltype, 
		 std::shared_ptr<c4::type> rtype) {
  bool cond = (ltype->is_scalar() && rtype->is_scalar());
  if(cond)
    bo->e_type() = c4::arithmetic_type::get_int();
  return cond;
}

bool ptr_arith(c4::binary_operator* bo, 
	       std::shared_ptr<c4::type> ltype, 
	       std::shared_ptr<c4::type> rtype) {
  bool cond = (ltype->pointer_to_complete() && rtype->is_arithmetic());
  if(cond)
    bo->e_type() = ltype;
  return cond;
}

bool equal_comp_ptrs(c4::binary_operator* bo, 
		     std::shared_ptr<c4::type> ltype, 
		     std::shared_ptr<c4::type> rtype) {
  bool cond = (ltype->pointer_to_complete() && rtype->pointer_to_complete()
	       && (*(as_pointer_type(ltype)->underlying())
		   == *(as_pointer_type(rtype)->underlying())));
  if(cond)
    bo->e_type() = c4::arithmetic_type::get_int();
  return cond;
}

bool equal_obj_ptrs(c4::binary_operator* bo, 
		    std::shared_ptr<c4::type> ltype, 
		    std::shared_ptr<c4::type> rtype) {
  bool cond = (ltype->pointer_to_object() && rtype->pointer_to_object()
	       && (*(as_pointer_type(ltype)->underlying())
		   == *(as_pointer_type(rtype)->underlying())));
  if(cond)
    bo->e_type() = c4::arithmetic_type::get_int();
  return cond;
}
  
bool ptr_null(c4::binary_operator* bo, 
	      std::shared_ptr<c4::type> ltype, 
	      std::shared_ptr<c4::type> rtype) {
  bool cond = ((ltype->is_pointer() && rtype->is_zero()) ||
	       (rtype->is_pointer() && ltype->is_zero()));
  if(cond)
    bo->e_type() = c4::arithmetic_type::get_int();
  return cond;
}


bool ternary_rcompatible
(const std::shared_ptr<c4::type>& ltype,
 const std::shared_ptr<c4::type>& rtype) {
  return
    // right operand is pointer and left operand is zero constant
    (rtype->is_pointer() && ltype->is_zero()) ||
    // left operand is pointer to object and right operand is pointer to void
    (ltype->pointer_to_object() && rtype->is_pointer() &&
     as_pointer_type(rtype)->underlying()->is_void());
}

bool ternary_lcompatible
(const std::shared_ptr<c4::type>& ltype,
 const std::shared_ptr<c4::type>& rtype) {
  return
    // both operands are void, same struct type, or pointers to same type
    (!(ltype->is_function()) && (*ltype == *rtype)) ||
    // left operand is pointer and right operand is zero constant OR
    // right operand is pointer to object and left operand is pointer to void
    ternary_rcompatible(rtype, ltype);
}

bool assign_compatible
(const std::shared_ptr<c4::type>& ltype,
 const std::shared_ptr<c4::type>& rtype) {

  auto rpoint = as_pointer_type(rtype);
  auto lpoint = as_pointer_type(ltype);
  return
    // both arithmetic
    (ltype->is_arithmetic() && rtype->is_arithmetic()) ||
    // pointers to same type
    (*ltype == *rtype) ||
    // left is pointer, right is zero constant
    (lpoint && rtype->is_zero()) ||
    // left is pointer to object and right is pointer to void
    (ltype->pointer_to_object() &&
     rpoint && rpoint->underlying()->is_void()) ||
    // right is pointer to object and left is pointer to void
    (rtype->pointer_to_object() &&
     lpoint && lpoint->underlying()->is_void());
}

bool is_lvalue(c4::expression* left) {
  c4::unary_operator* uleft;
  c4::primary_expression* pleft;
  return
    // is identifer
    ((pleft = dynamic_cast<c4::primary_expression*> (left)) &&
     pleft->value().type == c4::token_type::IDENTIFIER) ||
    // or postfix operator
    dynamic_cast<c4::postfix_operator*> (left) ||
    // or array access
    dynamic_cast<c4::subscript_operator*> (left) ||
    // or pointer dereference
    ((uleft = dynamic_cast<c4::unary_operator*> (left)) &&
     uleft->op() == c4::token_type::PCTR_STAR);
}



}
