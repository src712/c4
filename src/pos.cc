#include "pos.h"

#include <ostream>

std::ostream& c4::operator<<(std::ostream& o, const c4::Pos& p)
{ return o << p.name << ":" << p.line << ":" << p.column << ":"; }
