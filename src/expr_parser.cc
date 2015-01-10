#include "expr_parser.h"
#include "ast.h"
#include "decl_parser.h"
#include "diagnostic.h"

namespace {

int left_precedence(const c4::token& t) {
  switch(t.type) {
  case c4::token_type::PCTR_STAR: return 15;
  case c4::token_type::PCTR_PLUS:
  case c4::token_type::PCTR_MINUS: return 13;
  case c4::token_type::PCTR_LESS: return 11;
  case c4::token_type::PCTR_EQUAL:
  case c4::token_type::PCTR_NOT_EQUAL: return 9;
  case c4::token_type::PCTR_AND: return 7;
  case c4::token_type::PCTR_OR: return 5;
  case c4::token_type::PCTR_QUESTIONMARK: return 3;
  case c4::token_type::PCTR_ASSIGN: return 0;
  default: return -1;
  }
}

int right_precedence(const c4::token& t) {
  switch(t.type) {
  case c4::token_type::PCTR_STAR: return 14;
  case c4::token_type::PCTR_PLUS:
  case c4::token_type::PCTR_MINUS: return 12;
  case c4::token_type::PCTR_LESS: return 10;
  case c4::token_type::PCTR_EQUAL:
  case c4::token_type::PCTR_NOT_EQUAL: return 8;
  case c4::token_type::PCTR_AND: return 6;
  case c4::token_type::PCTR_OR: return 4;
  case c4::token_type::PCTR_QUESTIONMARK: return 2;
  case c4::token_type::PCTR_ASSIGN: return 1;
  default: return -1;
  }
}

} // unnamed

namespace c4 {

expression* expr_parser::operator()() {
  expression* left = match_operand();
  return match_subexpression(left, 0);
}

expression* expr_parser::match_operand() {
  return match_unary_expression();
}

expression* expr_parser::match_unary_expression() {
  expression* res;
  if(possibly(token_type::KWD_SIZEOF)) {
    auto size_pos = t().second;
    consume();
    if(possibly(token_type::PCTR_LPAREN)) {
      if(is_type_specifier(peek().first)) {
	consume();
	decl_parser dp{get_lexer()};
        Pos p = t().second;
        std::unique_ptr<type_specifier> ts{dp.match_type_specifier()};
        std::unique_ptr<declarator> dl{dp.match_declarator
	    (decl_parser::ABSTRACT)};
	res = new sizeof_type{
	  new type_name{p, ts.release(), dl.release()}, size_pos};
	expect(token_type::PCTR_RPAREN);
      } else {
      	res = new sizeof_expr{match_unary_expression(), size_pos}; 
      }
    } else {
      res =  new sizeof_expr{match_unary_expression(), size_pos};
    }
    return res;
  } else if(is_unary_operator(t().first)) {
    auto op = t();
    consume();
    return new unary_operator{op.first.type, match_unary_expression(), 
	op.second};
  } else {
    return match_postfix_expression();
  }
}

expression* expr_parser::match_postfix_expression() {
  expression* res = match_primary_expression();
  while(is_postfix_operator(t().first)) {
    auto op = t();
    consume();
    if(op.first.type == token_type::PCTR_DOT
       || op.first.type == token_type::PCTR_DEREF) {
      auto right = t();
      expect(token_type::IDENTIFIER);
      res = new postfix_operator{op.first.type, res, 
				 new primary_expression{right.first, 
							right.second}, 
				 op.second}; 
    } else if(op.first.type == token_type::PCTR_LBRACKET) {
      expression* right = (*this)();
      expect(token_type::PCTR_RBRACKET);
      res = new subscript_operator{res, right, op.second};
    } else { // op == (
      function_call* temp = new function_call{res, op.second};
      
      if(!possibly(token_type::PCTR_RPAREN) &&
	    !possibly(token_type::T_EOF)) {
	if(auto expr = (*this)())
	  temp->add_param(expr);
      }
      while(!possibly(token_type::PCTR_RPAREN) &&
	    !possibly(token_type::T_EOF)) {
	  expect(token_type::PCTR_COMMA);
	  if(auto expr = (*this)())
	    temp->add_param(expr);
      }
      expect(token_type::PCTR_RPAREN);
      res = temp;
    }
  }
  return res;
}

expression* expr_parser::match_primary_expression() {
  switch(t().first.type) {
  case token_type::IDENTIFIER:
  case token_type::INTEGER_CONSTANT:
  case token_type::CHARACTER_CONSTANT:
  case token_type::STRING_LITERAL: {
    auto res = t();
    consume();
    return new primary_expression{res.first, res.second};
  }
  case token_type::PCTR_LPAREN: {
    consume();
    expression* res = (*this)();
    expect(token_type::PCTR_RPAREN);
    return res;
  }
  default: { 
    errorf(t().second, "expected expression before '%s' token", 
	  token_to_string(t().first.type));
    return nullptr;
  }
  }
}

expression* expr_parser::match_subexpression(expression* left, int min_prec) {
  while(right_precedence(t().first) >= min_prec) {
    auto op = t();
    consume();
    if(is_ternary(op.first)) {
      expression* true_expr = (*this)();
      expect(token_type::PCTR_COLON);
      expression* false_expr = match_operand();
      false_expr = match_subexpression(false_expr, 2);
      left = new ternary_expr(left, true_expr, false_expr, op.second);
      continue;
    }
    expression* right = match_operand();
    while(right_precedence(t().first) > left_precedence(op.first)) {
      right = match_subexpression(right, right_precedence(t().first));
    }
    left =  new binary_operator{op.first.type, left, right, op.second};
  }
  return left;
}


} // c4
