#include "lexer.h"
#include <algorithm>

#include "munchers.h"
#include "diagnostic.h"

namespace {
template <typename Muncher>
void call_muncher(const char* begin, const char* end,
                  c4::token& cur, Muncher& m)
{
  auto x = m(begin, end);

  if(cur.type == c4::token_type::INVALID) {
    if(x.type != c4::token_type::INVALID) {
      cur = x;
    } else { // both are invalid
      if(x.data.size() > cur.data.size())
        cur = x;
    }
  } else {
    if(x.type != c4::token_type::INVALID) {
      if(x.data.size() > cur.data.size())
        cur = x;
    }
  }
}
}

namespace c4 {

void lexer::next_token(std::pair<token, Pos>& tokenarg, const char* end)
{
  assert(std::get<0>(tokenarg).type != token_type::COMMENT);
  // token_ is never a comment, so we don't need to think about advancing the line
  Pos& current_pos = tokenarg.second;
  current_pos.column += tokenarg.first.data.size();
  unsigned int& current_line = current_pos.line;
  unsigned int& current_col =  current_pos.column;

  // get our own begin, so we don't fiddle with the state
  const char* begin = std::get<0>(tokenarg).data.data() + std::get<0>(tokenarg).data.size();
  while(begin != end) {
    char c = *begin;
    if(c == ' ' || c == '\t' || c == '\v' || c == '\f') {
      // handle space, horizontal/vertical tab, and form feed
      ++current_col;
      ++begin;
    } else if(c == '\n' || c == '\r') {
      current_col = 1;
      ++current_line;
      ++begin;
      if(begin != end && ((c == '\n' && *begin == '\r')
			  || (c == '\r' && *begin == '\n'))) {
	++begin;
      }
    } else if(c == '\\') {
      ++current_col;
      ++begin;
      if(begin == end || (*begin == '\n'))  {
        current_col = 1;
        ++current_line;
        continue;
      } else {
        --current_col;
        errorf(current_pos, "stray '\\' in program"); // emit the error for the column before
        ++current_col;
      }
    } else if(std::iscntrl(c)) {
      // a stray control code!
      ++begin;
      errorf(current_pos, "stray '\\%d' in program", // TODO this will panic
             static_cast<int>(c));
    } else {
      punctuator_muncher pm;
      identifier_muncher im;
      decimal_muncher dm;
      string_muncher sm;
      comment_muncher cm;

      // this is not whitespace, send the marines
      tokenarg.first = token{token_type::INVALID, llvm::StringRef{}};
      call_muncher(begin, end, tokenarg.first, pm);
      call_muncher(begin, end, tokenarg.first, im);
      call_muncher(begin, end, tokenarg.first, dm);
      call_muncher(begin, end, tokenarg.first, sm);
      call_muncher(begin, end, tokenarg.first, cm);
      
      std::for_each(std::get<0>(tokenarg).errors.begin(), std::get<0>(tokenarg).errors.end(),
                    [&tokenarg](const std::string& x) { errorf(std::get<1>(tokenarg), x.c_str()); });

      if(std::get<0>(tokenarg).type == token_type::INVALID) {
	errorf(current_pos, "stray '%c' in program", c);
	++current_col;
	++begin;
	continue;
      } else if(std::get<0>(tokenarg).type != token_type::COMMENT) {
        return ;
      } else {
        // we do not ever store those tokens, but we need to look at
        // how many lines and columns they could have swallowed
	if(cm.lines > 0)
	  current_col = 1;
       	current_line += cm.lines;
	current_col += cm.cols;

        // skip the token
        begin = std::get<0>(tokenarg).data.data() + std::get<0>(tokenarg).data.size();
      }
    }
  }
  tokenarg.first = token{token_type::T_EOF, llvm::StringRef{end, 0}};
  return ;
}

}
