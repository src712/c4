#ifndef POS_H
#define POS_H

#include <iosfwd>

#include "util.h"

namespace c4 {

struct Pos
{
  explicit Pos(char const* const name) : Pos(name, 0, 0) {}

  Pos(char const* const name, unsigned const line, unsigned const column) :
    name(nonNull(name)), line(line), column(column)
  {}

  char const* name;
  unsigned    line;
  unsigned    column;
};

std::ostream& operator<<(std::ostream&, const Pos&);

} // c4
#endif
