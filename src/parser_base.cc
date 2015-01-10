#include "parser_base.h"
#include "diagnostic.h"

bool c4::parser_base::expect(const token_type& x) {
  if(x != t().first.type) {
    errorf(t().second, "expected token '%s', got '%s'", token_to_string(x),
           token_to_string(t().first.type));
    if(t().first.type == token_type::PCTR_COMMA)
      consume();
    return false;
  } else {
    consume();
    return true;
  }
}
