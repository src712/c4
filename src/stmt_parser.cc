#include "stmt_parser.h"
#include "ast.h"
#include "decl_parser.h"
#include "expr_parser.h"

namespace {
bool is_jump_statement(const c4::token& t) {
  switch(t.type) {
  case c4::token_type::KWD_GOTO:
  case c4::token_type::KWD_BREAK:
  case c4::token_type::KWD_CONTINUE:
  case c4::token_type::KWD_RETURN: return true;
  default: return false;
  }
}
}

namespace c4 {

stmt* stmt_parser::operator()() {
  if(possibly(token_type::PCTR_LBRACE)) 
    return match_compound();
  else if(possibly(token_type::IDENTIFIER)
	  && peek().first.type == token_type::PCTR_COLON)
    return match_labeled();
  else if(possibly(token_type::KWD_IF))
    return match_if();
  else if(possibly(token_type::KWD_WHILE))
    return match_while();
  else if(is_jump_statement(t().first))
    return match_jump();
  else
    return match_expression_statement();
}

comp_stmt* stmt_parser::match_compound() {
  expect(token_type::PCTR_LBRACE);
  comp_stmt* cs = new comp_stmt;
  while(!possibly(token_type::PCTR_RBRACE) && !possibly(token_type::T_EOF)) {
    if(is_type_specifier(t().first)) {
      decl_parser declp{get_lexer()};
      decl* dp = declp();
      cs->add_stmt(dp);
      expect(token_type::PCTR_SEMICOLON);
    } else {
      stmt* st = (*this)();
      cs->add_stmt(st);
    }
  }
  expect(token_type::PCTR_RBRACE);
  return cs;
}

labeled_stmt* stmt_parser::match_labeled() {
  std::string label = t().first.data;
  consume();
  auto pos = t().second;
  expect(token_type::PCTR_COLON);
  stmt* stmt = (*this)();
  return new labeled_stmt{label, stmt, pos};
}

stmt* stmt_parser::match_if() {
  auto pos = t().second;
  expect(token_type::KWD_IF);
  expect(token_type::PCTR_LPAREN);
  expr_parser ep{get_lexer()};
  expression* cond = ep();
  expect(token_type::PCTR_RPAREN);
  stmt* if_body = (*this)();
  if(possibly(token_type::KWD_ELSE)) {
    consume();
    return new if_else_stmt{cond, if_body, (*this)(), pos};
  }
  return new if_stmt{cond, if_body, pos};
}

while_stmt* stmt_parser::match_while() {
  auto pos = t().second;
  expect(token_type::KWD_WHILE);
  expect(token_type::PCTR_LPAREN);
  expr_parser ep{get_lexer()};
  expression* cond = ep();
  expect(token_type::PCTR_RPAREN);
  stmt* body = (*this)();
  return new while_stmt{cond, body, pos};
}

stmt* stmt_parser::match_jump() {
  stmt* jump = nullptr;
  if(possibly(token_type::KWD_GOTO)) {
    auto pos = t().second;
    consume();
    if(possibly(token_type::IDENTIFIER))
      jump = new goto_stmt{t().first.data, pos};
    expect(token_type::IDENTIFIER);
  } else if(possibly(token_type::KWD_RETURN)) {
    auto ret_pos = t().second;
    consume();
    if(!possibly(token_type::PCTR_SEMICOLON)) {
      expr_parser ep{get_lexer()};
      jump = new return_stmt{ep(), ret_pos};
    } else
      jump = new return_stmt{ret_pos};
  } else if(possibly(token_type::KWD_CONTINUE)) {
    jump = new continue_stmt{t().second};
    consume();
  } else {
    jump = new break_stmt{t().second};
    consume();
  }
  expect(token_type::PCTR_SEMICOLON);
  return jump;
}

expr_stmt* stmt_parser::match_expression_statement() {
  if(possibly(token_type::PCTR_SEMICOLON)) {
    consume();
  } else {
    expr_parser ep{get_lexer()};
    expression* expr = ep();
    if(!expect(token_type::PCTR_SEMICOLON) && possibly(token_type::PCTR_RPAREN))
      consume();
    return new expr_stmt{expr};
  }
  return new expr_stmt;
}

}
