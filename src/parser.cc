#include "parser.h"

#include "lexer.h"
#include "token.h"
#include "diagnostic.h"
#include "pos.h"
#include "ast.h"
#include "util.h"
#include "decl_parser.h"
#include "stmt_parser.h"

#include <cassert>
#include <memory>

c4::ast_node* c4::parser::get_ast_node() {
  if(possibly(token_type::T_EOF))
    errorf(t().second, "empty translation unit");
  std::unique_ptr<translation_unit> tu{new translation_unit{}};
  
  while(!possibly(token_type::T_EOF)) {
    decl_parser dp{get_lexer()};
    decl* declp = dp();
    tu->add_decl(declp);

    if(possibly(token_type::PCTR_LBRACE)) {
      // this must be a function definition, but there is still the
      // question of parameter list.
      stmt_parser sp{get_lexer()};
      stmt* s = sp();
      comp_stmt* cs = dynamic_cast<comp_stmt*>(s);
      if(cs != nullptr) {
        declp->set_body(cs);
      } else {
        errorf("ICE");
      }
    } else {
      // this must be a declaration
      expect(token_type::PCTR_SEMICOLON);
    }
  }

  return tu.release();
}
