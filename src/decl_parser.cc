#include "decl_parser.h"
#include "ast.h"

#include <vector>
#include <memory>

namespace c4 {

type_specifier* decl_parser::match_type_specifier() {
  if(!is_type_specifier(t().first)) {
    errorf(t().second, "expected a 'type-specifier', got '%s'", 
           token_to_string(t().first.type));
    return nullptr;
  }
    
  if(t().first.type == token_type::KWD_STRUCT 
     || t().first.type == token_type::KWD_UNION) {
    return match_struct();
  } else {
    auto x = new type_specifier(t().second, t().first.type);
    consume();
    return x;
  }
}

void decl_parser::match_struct_declaration_list(struct_specifier* s) {
  while(!possibly(token_type::PCTR_RBRACE) && !possibly(token_type::T_EOF)) {
    std::unique_ptr<decl> member{(*this)()};
    if(member->get_declarator() || 
       dynamic_cast<struct_specifier*>(member->get_type_specifier()))
      s->add_decl(member.release());
    expect(token_type::PCTR_SEMICOLON);
  }
}

decl* decl_parser::operator()() {
  Pos p = t().second;
  std::unique_ptr<type_specifier> ts{match_type_specifier()};
  std::unique_ptr<declarator> dl {match_declarator()};
  if(ts == nullptr && dl == nullptr) { // todo should this still be here?
    // neither of the parsers consumed input, consume it ourselves
    consume();
  }
  if(!dl && !dynamic_cast<struct_specifier*>(ts.get()))
    errorf(p, "declaration with empty declarator");
  return new decl{p, ts.release(), dl.release()};
}

struct_specifier* decl_parser::match_struct() {
  assert(possibly(token_type::KWD_STRUCT) || possibly(token_type::KWD_UNION));
  Pos structpos = t().second;
  auto type = t().first.type;
  struct_specifier* s =  nullptr;
  consume();
  auto tag = t();
  expect(token_type::IDENTIFIER);
  s = new struct_specifier{structpos, type, tag.first.data};
  
  if(possibly(token_type::PCTR_LBRACE)) {
    consume();
    match_struct_declaration_list(s);
    if(!s->decls().size())
      errorf(structpos, "struct has no members");
    expect(token_type::PCTR_RBRACE);
  }

  return s;
}

declarator* decl_parser::match_declarator(declarator_kind kind) {
  // parse pointers
  if(possibly(token_type::PCTR_STAR)) {
    consume();
    return new declarator{true, match_declarator(kind)};
  }

  // parse direct (abs) declarators
  declarator* dp = nullptr;
  if(possibly(token_type::PCTR_LPAREN)) {
    consume();
    // parse param list
    if((kind != NON_ABSTRACT) && (is_type_specifier(t().first) 
				  || possibly(token_type::PCTR_RPAREN))) {
      dp = new declarator{false, ""};
      match_parameter_type_list(dp);
      expect(token_type::PCTR_RPAREN);
      return dp;
    } else {
      dp = new declarator{false, match_declarator(kind)};
      expect(token_type::PCTR_RPAREN);
    }
  } else if(kind != ABSTRACT && possibly(token_type::IDENTIFIER)) {
    dp = new declarator{false, t().first.data};
    consume(); // todo: error when non-abstract and missing identifier
  }

  // parse param list
  if(possibly(token_type::PCTR_LPAREN)) {
    if(!dp)
      dp = new declarator{false, ""}; 
    consume();
    match_parameter_type_list(dp);
    expect(token_type::PCTR_RPAREN);
  }
  return dp;
}

void decl_parser::match_parameter_type_list(declarator* d) {
  if(possibly(token_type::PCTR_VARARGS))
    errorf(t().second, "require an argument before '...'");
  
  if(!possibly(token_type::PCTR_RPAREN) && !possibly(token_type::T_EOF))
    match_parameter_type(d);

  while(!possibly(token_type::PCTR_RPAREN) && !possibly(token_type::T_EOF)) {
    expect(token_type::PCTR_COMMA);
    match_parameter_type(d);
  }
  if(d->parameter_decls().empty())
    errorf(t().second, "empty parameter list not allowed");
}

void decl_parser::match_parameter_type(declarator* d) {
 if(possibly(token_type::PCTR_VARARGS)) {
   consume();
   if(!possibly(token_type::PCTR_RPAREN))
     errorf(t().second, "'...' needs to be the last argument");
 } else {
   Pos p = t().second;
   std::unique_ptr<type_specifier> ts{match_type_specifier()};
   std::unique_ptr<declarator> dl{match_declarator(BOTH)};
   d->add_parameter_decl(new parameter_decl{p, ts.release(), dl.release()});
 }
}
}
