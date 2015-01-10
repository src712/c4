#include "munchers.h"
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <cctype>
#include "llvm/ADT/Hashing.h"

namespace c4 {

token
string_muncher::operator()(const char* begin, const char* end) const
{
  // presorted
  static char escapes[]
    = {'\'', '\"', '?', '\\', 'a', 'b', 'f', 'n', 'r', 't', 'v'};

  auto is_escape = [](char c)
    { return std::find(std::begin(escapes), std::end(escapes), c) != std::end(escapes); };

  if(begin == end || (*begin != '\'' && *begin != '"'))
    return token{token_type::INVALID, llvm::StringRef{}};

  const char* backup = begin;
  bool terminated = false;
  std::vector<std::string> errs;
  char delim = *begin++;
  std::size_t length = 0;
  while(begin != end) {
    if(*begin == delim) {
      ++begin;
      terminated = true;
      break;
    } else if(*begin == '\n' || *begin == '\r') {
      break;
    }

    if(*begin == '\\') {
      // read one more and check the escape
      ++begin;
      if(begin == end) { // eof
        break;
      }
      if(*begin == '\n' || *begin == '\r') {
        // this can never happen, because the preprocessor took care of it
        break;
      }

      if(!is_escape(*begin)) {
        errs.push_back("unknown escape sequence \\");
        errs.back().push_back(*begin);
      }
    }
    ++begin;
    ++length;
  }

  if(!terminated) {
    if(delim == '\'')
      errs.emplace_back("missing terminating ' character");
    else
      errs.emplace_back("missing terminating \" character");
  }

  if(delim == '\'' && length != 1) {
    errs.emplace_back("invalid length character constant");
  }

  if(delim == '\'')
    return token{token_type::CHARACTER_CONSTANT, llvm::StringRef{backup, static_cast<std::size_t>(begin - backup)},
        std::move(errs)};
  else
    return token{token_type::STRING_LITERAL, llvm::StringRef{backup, static_cast<std::size_t>(begin - backup)},
        std::move(errs)};
}

token
comment_muncher::impl(const char* begin, const char* end)
{
  const char* start = begin;
  ++begin;
  bool fcstyle = (*begin == '*');
  ++begin;
  cols = 2;

  if(!fcstyle) {
    while(begin != end && *begin != '\n' && *begin != '\r')
      ++begin;
    return token{token_type::COMMENT, llvm::StringRef{start, static_cast<std::size_t>(begin - start)}};
  } else {
    bool star = (*begin == '*');
    while(begin != end && !(star && *begin == '/')) {
      star = (*begin == '*');
      if(*begin == '\n' || *begin == '\r') {
        ++lines; cols = 0;
        auto it = std::next(begin);
        if(it != end && ((*begin == '\n' && *it == '\r') || (*begin == '\r' && *it == '\n')))
          begin = ++it;
        else
          ++begin;
      } else {
        ++begin;
        ++cols;
      }
    }

    if(begin != end) { // this comment is terminated
      auto t = token{token_type::COMMENT, llvm::StringRef{start, static_cast<std::size_t>(begin - start + 1)}};
      ++cols;
      return t;
    } else { // this comment is unterminated
      auto t = token{token_type::COMMENT, llvm::StringRef{start, static_cast<std::size_t>(begin - start)}};
      t.errors.emplace_back("unterminated comment");
      return t;
    }
  }
}

token
decimal_muncher::operator()(const char* b, const char* end) const
{
  if(b == end) return token{token_type::INVALID, llvm::StringRef{}};

  char ch = *b;
  if (!std::isdigit(ch))
    return token{token_type::INVALID, llvm::StringRef{}};
  if(ch == '0')
    return token{token_type::INTEGER_CONSTANT, llvm::StringRef{b, 1}};

  const char* begin = b;

  while(++begin != end) {
    if (!std::isdigit(*begin))
      break;
  }

  return token{token_type::INTEGER_CONSTANT, llvm::StringRef{b, static_cast<std::size_t>(begin - b)}};
}

token
identifier_muncher::operator()(const char* b, const char* end) const
{
  auto isnondigit = [](const char& c)
    { return std::isalpha(c) || c == '_'; };
  auto isnondigitordigit = [](const char& c)
    { return std::isalnum(c) || c == '_'; };

  static auto shash = [](const llvm::StringRef& r) -> std::size_t { return llvm::hash_value(r); };
  static std::unordered_map<llvm::StringRef, token_type, decltype(shash)>
    kwd_map = {
    {{"auto", token_type::KWD_AUTO}, {"break", token_type::KWD_BREAK}, {"case", token_type::KWD_CASE},
     {"char", token_type::KWD_CHAR}, {"const", token_type::KWD_CONST}, {"continue", token_type::KWD_CONTINUE},
     {"default", token_type::KWD_DEFAULT}, {"do", token_type::KWD_DO}, {"double", token_type::KWD_DOUBLE},
     {"else", token_type::KWD_ELSE}, {"enum", token_type::KWD_ENUM}, {"extern", token_type::KWD_EXTERN},
     {"float", token_type::KWD_FLOAT}, {"for", token_type::KWD_FOR}, {"goto", token_type::KWD_GOTO},
     {"if", token_type::KWD_IF}, {"inline", token_type::KWD_INLINE}, {"int", token_type::KWD_INT},
     {"long", token_type::KWD_LONG}, {"register", token_type::KWD_REGISTER},
     {"restrict", token_type::KWD_RESTRICT}, {"return", token_type::KWD_RETURN}, {"short", token_type::KWD_SHORT},
     {"signed", token_type::KWD_SIGNED}, {"sizeof", token_type::KWD_SIZEOF}, {"static", token_type::KWD_STATIC},
     {"struct", token_type::KWD_STRUCT}, {"switch", token_type::KWD_SWITCH}, {"typedef", token_type::KWD_TYPEDEF},
     {"union", token_type::KWD_UNION}, {"unsigned", token_type::KWD_UNSIGNED}, {"void", token_type::KWD_VOID},
     {"volatile", token_type::KWD_VOLATILE}, {"while", token_type::KWD_WHILE}, {"_Alignas", token_type::KWD__ALIGNAS},
     {"_Alignof", token_type::KWD__ALIGNOF}, { "_Atomic", token_type::KWD__ATOMIC}, {"_Bool", token_type::KWD__BOOL},
     {"_Complex", token_type::KWD__COMPLEX}, {"_Generic", token_type::KWD__GENERIC},
     {"_Imaginary", token_type::KWD__IMAGINARY}, {"_Noreturn", token_type::KWD__NORETURN},
     {"_Static_assert", token_type::KWD__STATIC_ASSERT}, {"_Thread_local", token_type::KWD__THREAD_LOCAL}},
    static_cast<int>(token_type::KWD__THREAD_LOCAL) - static_cast<int>(token_type::KWD_AUTO), shash
  }; // map from keywords to the respective enums


  if(b == end || !isnondigit(*b))
    return token{token_type::INVALID, llvm::StringRef{}};

  const char* begin = b;

  while(begin != end) {
    if(isnondigitordigit(*begin))
      ++begin;
    else
      break;
  }

  llvm::StringRef ref{b, static_cast<std::size_t>(begin - b)};
  auto it = kwd_map.find(ref);
  if(it != kwd_map.end())
    return token{it->second, ref};
  else
    return token{token_type::IDENTIFIER, ref};
}

token
punctuator_muncher::operator()(const char* begin, const char* end) const
{
  const char* start = begin;
  switch(*begin) {
  case '[': return token{token_type::PCTR_LBRACKET, llvm::StringRef{start, 1}};
  case ']': return token{token_type::PCTR_RBRACKET, llvm::StringRef{start, 1}};
  case '(': return token{token_type::PCTR_LPAREN, llvm::StringRef{start, 1}};
  case ')': return token{token_type::PCTR_RPAREN, llvm::StringRef{start, 1}};
  case '{': return token{token_type::PCTR_LBRACE, llvm::StringRef{start, 1}};
  case '}': return token{token_type::PCTR_RBRACE, llvm::StringRef{start, 1}};
  case ';': return token{token_type::PCTR_SEMICOLON, llvm::StringRef{start, 1}};
  case '~': return token{token_type::PCTR_TILDE, llvm::StringRef{start, 1}};
  case ',': return token{token_type::PCTR_COMMA, llvm::StringRef{start, 1}};
  case '?': return token{token_type::PCTR_QUESTIONMARK, llvm::StringRef{start, 1}};
  case '*': ++begin; if(begin != end && *begin == '=') { return token{token_type::PCTR_MULT_ASSIGN, llvm::StringRef{start, 2}}; }
    else return token{token_type::PCTR_STAR, llvm::StringRef{start, 1}};
  case '=': ++begin; if(begin != end && *begin == '=') { return token{token_type::PCTR_EQUAL, llvm::StringRef{start, 2}}; }
    else return token{token_type::PCTR_ASSIGN, llvm::StringRef{start, 1}};
  case '#': ++begin; if(begin != end && *begin == '#') { return token{token_type::PCTR_HASHASH, llvm::StringRef{start, 2}}; }
    else return token{token_type::PCTR_HASH, llvm::StringRef{start, 1}};
  case '!': ++begin; if(begin != end && *begin == '=') { return token{token_type::PCTR_NOT_EQUAL, llvm::StringRef{start, 2}}; }
    else return token{token_type::PCTR_BANG, llvm::StringRef{start, 1}};
  case '/': ++begin; if(begin != end && *begin == '=') { return token{token_type::PCTR_DIV_ASSIGN, llvm::StringRef{start, 2}}; }
    else return token{token_type::PCTR_SLASH, llvm::StringRef{start, 1}};
  case '^': ++begin; if(begin != end && *begin == '=') { return token{token_type::PCTR_POWER_ASSIGN, llvm::StringRef{start, 2}}; }
    else return token{token_type::PCTR_POWER, llvm::StringRef{start, 1}};
  case ':': ++begin; if(begin != end && *begin == '>') { return token{token_type::PCTR_RBRACKET, llvm::StringRef{start, 2}}; }
    else return token{token_type::PCTR_COLON, llvm::StringRef{start, 1}};
  case '.': {
    ++begin;
    if(begin != end && *begin == '.') {
      ++begin;
      if(begin != end && *begin == '.') return token{token_type::PCTR_VARARGS, llvm::StringRef{start, 3}};
      else return token{token_type::PCTR_DOT, llvm::StringRef{start, 1}};
    } else {
      return token{token_type::PCTR_DOT, llvm::StringRef{start, 1}};
    }
  }
  case '&': {
    ++begin;
    if(begin != end && *begin == '&')
      return token{token_type::PCTR_AND, llvm::StringRef{start, 2}};
    else if(begin != end && *begin == '=')
      return token{token_type::PCTR_BIT_AND_ASSIGN, llvm::StringRef{start, 2}};
    else
      return token{token_type::PCTR_BIT_AND, llvm::StringRef{start, 1}};
  }
  case '+': {
    ++begin;
    if(begin != end && *begin == '+')
      return token{token_type::PCTR_INCREMENT, llvm::StringRef{start, 2}};
    else if(begin != end && *begin == '=')
      return token{token_type::PCTR_PLUS_ASSIGN, llvm::StringRef{start, 2}};
    else
      return token{token_type::PCTR_PLUS, llvm::StringRef{start, 1}};
  }
  case '-': {
    ++begin;
    if(begin != end && *begin == '-')
      return token{token_type::PCTR_DECREMENT, llvm::StringRef{start, 2}};
    else if(begin != end && *begin == '=')
      return token{token_type::PCTR_MINUS_ASSIGN, llvm::StringRef{start, 2}};
    else if(begin != end && *begin == '>')
      return token{token_type::PCTR_DEREF, llvm::StringRef{start, 2}};
    else
      return token{token_type::PCTR_MINUS, llvm::StringRef{start, 1}};
  }
  case '%': {
    ++begin;
    if(begin != end && *begin == '=') {
      return token{token_type::PCTR_MOD_ASSIGN, llvm::StringRef{start, 2}};
    } else if(begin != end && *begin == '>') {
      return token{token_type::PCTR_RBRACE, llvm::StringRef{start, 2}};
    } else if(begin != end && *begin == ':') {
      ++begin;
      if(begin != end && *begin == '%') {
        ++begin;
        if(begin != end && *begin == ':') {
          return token{token_type::PCTR_HASHASH, llvm::StringRef{start, 4}};
        } else {
          return token{token_type::PCTR_HASH, llvm::StringRef{start, 2}};
        }
      }
      return token{token_type::PCTR_HASH, llvm::StringRef{start, 2}};
    } else {
      return token{token_type::PCTR_MOD, llvm::StringRef{start, 1}};
    }
  }
  case '<': {
    ++begin;
    if(begin != end && *begin == '=')
      return token{token_type::PCTR_LESS_THAN, llvm::StringRef{start, 2}};
    else if(begin != end && *begin == ':')
      return token{token_type::PCTR_LBRACKET, llvm::StringRef{start, 2}};
    else if(begin != end && *begin == '%')
      return token{token_type::PCTR_LBRACE, llvm::StringRef{start, 2}};
    else if(begin != end && *begin == '<') {
      ++begin;
      if(begin != end && *begin == '=') {
        return token{token_type::PCTR_SHIFT_LEFT_ASSIGN, llvm::StringRef{start, 3}};
      }
      return token{token_type::PCTR_SHIFT_LEFT, llvm::StringRef{start, 2}};
    } else
      return token{token_type::PCTR_LESS, llvm::StringRef{start, 1}};
  }
  case '>': {
    ++begin;
    if(begin != end && *begin == '=')
      return token{token_type::PCTR_GREATER_THAN, llvm::StringRef{start, 2}};
    else if(begin != end && *begin == '>') {
      ++begin;
      if(begin != end && *begin == '=') {
        return token{token_type::PCTR_SHIFT_RIGHT_ASSIGN, llvm::StringRef{start, 3}};
      }
      return token{token_type::PCTR_SHIFT_RIGHT, llvm::StringRef{start, 2}};
    } else
      return token{token_type::PCTR_GREATER, llvm::StringRef{start, 1}};
  }
  case '|': {
    ++begin;
    if(begin != end && *begin == '|')
      return token{token_type::PCTR_OR, llvm::StringRef{start, 2}};
    else if(begin != end && *begin == '=')
      return token{token_type::PCTR_BIT_OR_ASSIGN, llvm::StringRef{start, 2}};
    else
      return token{token_type::PCTR_BIT_OR, llvm::StringRef{start, 1}};
  }
  default: return token{token_type::INVALID, llvm::StringRef{start, 0}};
  }
}

}
