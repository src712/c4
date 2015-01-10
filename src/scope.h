#ifndef C4_SCOPE_H
#define C4_SCOPE_H

#include "diagnostic.h"
#include "ast.h"
#include "type.h"

#include <iosfwd>
#include <vector>
#include <string>
#include <algorithm>
#include <memory>

namespace c4 {

struct scope {
  //! Enter a new scope.
  void enter_scope() { 
    if(ignore_ == scope_.size()) { ++ignore_; return; }
    if(ignore_ != 0) ++ignore_;
    scope_.emplace_back(nullptr);
    tag_scope_.emplace_back(nullptr); 
  }
  
  //! Ignore the next matching pair of enter/leave_scope commands.
  void ignore_next() { 
    assert(!scope_.empty()); // stack is not empty
    ignore_ = scope_.size();
  }

  //! Leave the current scope.
  void leave_scope() {
    if(ignore_ != 0) --ignore_;
    if(ignore_ == scope_.size()) { ignore_ = 0; return; } // the scope we want to ignore

    // Find the last nullptr and erase from there to the end.
    auto it = std::find(scope_.rbegin(), scope_.rend(), nullptr);
    if(it == scope_.rend())
      errorf("ICE: tried to pop top-level scope.");
    scope_.erase(--it.base(), scope_.end()); // classic reverse_iterator magick

    auto tit = std::find(tag_scope_.rbegin(), tag_scope_.rend(), nullptr);
    if(tit == tag_scope_.rend())
      errorf("ICE: tried to pop top-level scope.");
    tag_scope_.erase(--tit.base(), tag_scope_.end()); // classic reverse_iterator magick
  }
  
  //! Insert a new decl into the current scope. Returns `true` if
  //! successful, `false` if the name already exists in the current
  //! scope.
  bool put(base_decl* d) {
    assert(d);
    for(auto it = scope_.rbegin(); it != scope_.rend(); ++it) {
      if(*it == nullptr) {
        break;
      } else if((*it)->get_name() == d->get_name()) {
        // ouch, identifier already exists in the current scope
        return false;
      }
    }
    // We reached the end of the top-level scope, good to add.
    if(ignore_ != 0) ++ignore_;
    scope_.emplace_back(d); 
    return true;
  }
  
  void put(std::shared_ptr<struct_type> s) {
    tag_scope_.emplace_back(s);
  }

  //! Retrieve a struct_specifier by tag.
  std::shared_ptr<struct_type> get_tag(const std::string& tag) {
    auto it = std::find_if(tag_scope_.rbegin(), tag_scope_.rend(), 
                           [&tag](std::shared_ptr<struct_type> d) { 
			     return d ? d->get_tag() == tag : false; });
    return it != tag_scope_.rend() ? *it : nullptr;
  }

  //! Only look in the current scope for `tag`.
  std::shared_ptr<struct_type> get_tag_in_current_scope
  (const std::string& tag) const {
    auto it = std::find_if(tag_scope_.rbegin(), tag_scope_.rend(), 
                           [&tag](std::shared_ptr<struct_type> d) { 
			     return d ? d->get_tag() == tag : true; });
    return it == tag_scope_.rend() || *it == nullptr ? nullptr : *it;
  }

  //! Retrieve a decl by name.
  base_decl* get(const std::string& name) const
  { 
    // search backwards, because we want the latest name. account for nullptr.
    auto it = std::find_if(scope_.rbegin(), scope_.rend(), 
                           [&name](base_decl* d) { 
			     return d ? d->get_name() == name : false; });
    return it != scope_.rend() ? *it : nullptr;
  }

  //! Only look in the current scope for `name`.
  base_decl* get_in_current_scope(const std::string& name) const {
    auto it = std::find_if(scope_.rbegin(), scope_.rend(), 
                           [&name](base_decl* d) { 
			     return d ? d->get_name() == name : true; });
    return it == scope_.rend() || *it == nullptr ? nullptr : *it;
  }

  //! Dump a scope string representation into a ostream.
  friend std::ostream&
  operator<<(std::ostream&, const scope&);

  
private:
  std::vector<base_decl*> scope_;
  std::vector<std::shared_ptr<struct_type>> tag_scope_;
  std::size_t ignore_ = 0;
};

}

#endif /* C4_SCOPE_H */
