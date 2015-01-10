#ifndef C4_PARSER_BASE_H
#define C4_PARSER_BASE_H

#include "util.h"
#include "token.h"
#include "pos.h"
#include "lexer.h"
#include "diagnostic.h"

#include <algorithm>

namespace c4 {

struct parser_base
{
  parser_base(lexer* l) : l_(nonNull(l)) {}
protected:
  using token_pos_pair = std::pair<token, Pos>;
  
  // return if cur_token.first.type equals t. advance the token in
  // case of a match and error otherwise.
  bool expect(const token_type& t);

  // return if cur_token.first.type equals t. don't advance the
  // current state in any case.
  inline bool possibly(const token_type& x) const
  { return x == t().first.type; }

  inline void consume() 
  { l_->advance_token(); }

  inline token_pos_pair peek() 
  { return l_->peek(); }
  
  token_pos_pair t() const { return l_->get_token(); }

  lexer* get_lexer() { return l_; }
private:
  lexer* l_;
};

} // c4

#endif /* C4_PARSER_BASE_H */
