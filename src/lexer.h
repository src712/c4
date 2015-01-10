#ifndef C4_LEXER_H_
#define C4_LEXER_H_

#include <utility>
#include <tuple>

#include "pos.h"
#include "token.h"
#include "diagnostic.h"

namespace c4 {

class lexer
{
  static void next_token(std::pair<token, Pos>&, const char*);
public:
  lexer(const char* begin, const char* end, const char* filename) 
    : end_(end),
      token_(token{token_type::INVALID, llvm::StringRef{begin, 0}}, Pos{filename, 1, 1})
  {
    next_token(token_,end);
  }

  //! Get the current token.
  std::pair<token, Pos> get_token() const
  { return token_; }

  //! Advance to the next token and return it.
  std::pair<token, Pos> advance_token()
  { next_token(token_, end_); return get_token(); }

  //! Look at the next token without changing the current token.
  std::pair<token, Pos> peek() const // advance a copy
  { auto t = token_; next_token(t, end_); return t; }

private:
  const char* end_;
  std::pair<token, Pos> token_;
};

} // c4
#endif /* C4_LEXER_H_ */
