#ifndef C4_PARSER_H
#define C4_PARSER_H

#include <utility>
#include "pos.h"
#include "token.h"
#include "util.h"
#include "parser_base.h"

namespace c4 {

struct ast_node;

class parser : parser_base
{
public:
  using parser_base::parser_base;
  ast_node* get_ast_node();
};

} // c4
#endif /* C4_PARSER_H */
