#include "scope.h"

namespace c4 {

std::ostream&
operator<<(std::ostream& o, const scope& s) {
  int tabs = 0;
  o << "{\n";
  auto print_tabs = [&o, &tabs]() {
      for(int i = 0; i < tabs; ++i) {
        o << "\t";
      }
  };
  
  for(auto& d : s.scope_) {
    if(d == nullptr) {
      ++tabs;
      print_tabs();
      o << "{\n";
    } else {
      for(int i = 0; i < tabs; ++i) {
        o << "\t";
      }
      o << d->get_name() << '\n';
    }
  }
  while(tabs != 0) {
    print_tabs();
    o << "}\n";
    --tabs;
  }
  o << '}'; // don't print my own newline
  return o;
}

}
