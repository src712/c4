#ifndef C4_MUNCHERS_
#define C4_MUNCHERS_

#include "token.h"

namespace c4 {

struct string_muncher
{
  token
  operator()(const char* begin, const char* end) const;
};

struct comment_muncher
{
  token
  impl(const char* begin, const char* end);

  token
  operator()(const char* begin, const char* end) 
  {
    if(begin == end || *begin != '/') return token{token_type::INVALID, llvm::StringRef{}};
    auto next = std::next(begin);
    if(next == end || (*next != '/' && *next != '*'))
      return token{token_type::INVALID, llvm::StringRef{}};

    return impl(begin, end);
  }


  unsigned int cols = 0;
  unsigned int lines = 0;
};

struct decimal_muncher
{
  token
  operator()(const char* begin, const char* end) const;
};

struct identifier_muncher
{
  token
  operator()(const char* begin, const char* end) const;
};

struct punctuator_muncher
{
  token
  operator()(const char* begin, const char* end) const;
};

} // c4

#endif /* C4_MUNCHERS_ */
