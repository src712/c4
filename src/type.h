#ifndef C4_TYPE_H
#define C4_TYPE_H

#include "token.h"
#include <vector>
#include <algorithm>
#include <cassert>
#include <memory>
#include <ostream>

#include "llvm/IR/Type.h"
#include "llvm/IR/IRBuilder.h"

namespace c4 {

struct type {
  type() = default;
  // no copy, no move.
  type(const type&) = delete;
  type& operator=(const type&) = delete;
  type(type&&) = delete;
  type& operator=(type&&) = delete;
  
  virtual bool is_arithmetic() const { return false; }
  virtual bool is_zero() const { return false; }
  virtual bool is_void() const { return false; } 
  virtual bool is_function() const { return false; }
  virtual bool is_struct() const { return false; }
  virtual bool is_pointer() const { return false; }
  virtual bool is_object() const { return false; }
  virtual bool is_scalar() const { return false; }
  virtual bool is_complete() const = 0;
  virtual bool is_error() const { return false; }
  virtual bool pointer_to_complete() { return false; }
  virtual bool pointer_to_object() { return false; }

  virtual bool is_int() const { return false; }
  virtual bool is_char() const { return false; }

  virtual bool equals(const type&) const =0;
  virtual std::ostream& print(std::ostream&) const =0;

  virtual llvm::Type* llvm_type(llvm::IRBuilder<>&) { return nullptr; }
  virtual ~type() =0;
};

inline bool operator==(const type& lhs, const type& rhs)
{ return lhs.equals(rhs); }

inline bool operator!=(const type& lhs, const type& rhs)
{ return !lhs.equals(rhs); }

inline std::ostream& operator<<(std::ostream& os, const type& rhs)
{ return rhs.print(os); }

inline type::~type() {}

struct object_type : type {
  virtual bool is_object() const override final { return true; }
};

struct scalar_type : object_type {
  virtual bool is_complete() const override { return true; }
  virtual bool is_scalar() const override final { return true; }
};

struct pointer_type : scalar_type {
 pointer_type(std::shared_ptr<type> underlying) 
   : underlying_{underlying} {}
   
  // observing pointer to the underlying type
  std::shared_ptr<type> underlying() const { return underlying_; }

  bool is_pointer() const override final { return true; }
  bool pointer_to_complete() {
    return underlying_ && underlying_->is_complete();
  }
  bool pointer_to_object() {
    return underlying_ && underlying_->is_object();
  }

  bool equals(const type& o) const override {
    auto oc = dynamic_cast<const pointer_type*>(&o);
    if(!oc) return false; // o is no a pointer
    return underlying_->equals(*(oc->underlying()));
  }

  std::ostream& print(std::ostream& os) const override {
    return os << "pointer to " << *underlying_;
  }

  virtual llvm::Type* llvm_type(llvm::IRBuilder<>& b) override {
    if(underlying_->is_char() || underlying_->is_void()) {
      return b.getInt8PtrTy();
    }
    
    return llvm::PointerType::getUnqual(underlying_->llvm_type(b));
  }

private:
  std::shared_ptr<type> underlying_;
};

struct void_type : object_type {
  static std::shared_ptr<void_type> get_void() {
    static std::shared_ptr<void_type> instance(new void_type);
    return instance;
  }

  bool is_void() const override final { return true; }
  bool is_complete() const override final { return false; }
 
  bool equals(const type& o) const override 
  { return dynamic_cast<const void_type*>(&o); }
  
  std::ostream& print(std::ostream& os) const override {
    return os << "void";
  }
  virtual llvm::Type* llvm_type(llvm::IRBuilder<>& b) override
  { return b.getVoidTy(); }

private:
  void_type() {}
};

struct arithmetic_type : scalar_type {
  static std::shared_ptr<arithmetic_type> get_int() {
    static std::shared_ptr<arithmetic_type> 
      int_instance(new arithmetic_type{token_type::KWD_INT});
    return int_instance;
  }
  static std::shared_ptr<arithmetic_type> get_char() {
    static std::shared_ptr<arithmetic_type> 
      char_instance(new arithmetic_type{token_type::KWD_CHAR});
    return char_instance;
  }
  static std::shared_ptr<arithmetic_type> get_zero() {
    static std::shared_ptr<arithmetic_type> 
      zero_instance(new arithmetic_type{token_type::KWD_INT, true});
    return zero_instance;
  }

  bool is_zero() const override final { return zero_; }
  bool is_arithmetic() const override final { return true; }
  bool is_char() const override { return tt_ == token_type::KWD_CHAR; }
  bool is_int() const override { return tt_ == token_type::KWD_INT; }

  bool equals(const type& o) const override {
    auto oa = dynamic_cast<const arithmetic_type*>(&o);
    if(!oa) return false; // not an arithmetic_type
    return this->tt_ == oa->tt_;
  }

  std::ostream& print(std::ostream& os) const override {
    switch(tt_) {
    case token_type::KWD_INT:
      if(zero_)
        return os << "NULLTYPE";
      else 
        return os << "int";
    case token_type::KWD_CHAR:
      return os << "char";
    default:
      return os;
    }
  }

  llvm::Type* llvm_type(llvm::IRBuilder<>& b) {
    switch(tt_) {
    case token_type::KWD_INT:
      return b.getInt32Ty();
    case token_type::KWD_CHAR:
      return b.getInt8Ty();
    default:
      return nullptr;
    }
  }
private:
  arithmetic_type(token_type tt, bool zero = false) : tt_{tt}, zero_{zero}
  { 
    assert(tt_ == token_type::KWD_INT  || 
           tt_ == token_type::KWD_CHAR);
  }
  token_type tt_;
  bool zero_;
};

struct struct_type : object_type {
  typedef std::vector< std::pair< std::string, std::shared_ptr<type> > > member_list;
  typedef member_list::value_type value_type;

  template<typename... Members>
  struct_type(const std::string& tag, Members... mems)
    : members_{mems...}, tag_{tag} {}
  
  bool is_struct() const override final { return true; }
  bool is_complete() const override { return !members_.empty(); }

  const member_list& members()
  { return members_; }
  const member_list& members() const
  { return members_; }

  // Return a std::shared_ptr<type> for the identifier `s`. The
  // shared_ptr will be null if there is no such identifier.
  value_type::second_type lookup(const value_type::first_type& s)
  { 
    auto it = std::find_if(begin(members_), end(members_),
                           [&s](const value_type& v) { return s == v.first; });
    if(it != end(members_)) {
      return it->second;
    } else {
      return value_type::second_type{nullptr};
    }
  }

  int index_of(const value_type::first_type& s)
  {
    auto it = std::find_if(begin(members_), end(members_),
                           [&s](const value_type& v) { return s == v.first; });
    assert(it != end(members_));
    return std::distance(members_.begin(), it);
  }

  const std::string& get_tag() const { return tag_; }

  void add_member(const value_type& th) 
  { members_.emplace_back(th); }

  bool equals(const type& o) const override {
    return this == &o;
  }

  std::ostream& print(std::ostream& os) const override {
    os << "struct " << get_tag() << "{ ";
    return is_complete() ?
      os << "complete }" :
      os << "incomplete }";
  }

  virtual llvm::Type* llvm_type(llvm::IRBuilder<>& b) override {
    if(llvm_type_)
      return llvm_type_;
    auto struct_type = llvm::StructType::create(b.getContext(), 
                                                std::string{"struct."} + 
						get_tag());
    std::vector<llvm::Type*> member_types;
    member_types.reserve(members().size());
    for(auto& t : members()) {
      member_types.emplace_back(t.second->llvm_type(b));
    }
    struct_type->setBody(std::move(member_types));
    llvm_type_ = struct_type;
    return struct_type;
  }
private: 
  llvm::Type* llvm_type_ = nullptr;
  member_list members_;
  std::string tag_;
};

struct function_type : type {
  typedef std::vector<std::shared_ptr<type>> argument_list;

  template<typename... Arguments>
  function_type(const std::shared_ptr<type>& return_type, Arguments... args) 
    : return_type_{return_type}, arguments_{args...}
  {}

  bool is_function() const override final { return true; } 
  bool is_complete() const override final { return false; }

  std::shared_ptr<type> return_type() { return return_type_; }
  const std::shared_ptr<type> return_type() const { return return_type_; }

  bool is_nullary() const {
    return arguments_.size() == 1 && arguments_.front()->is_void();
  }

  std::size_t argument_size() const {
    if(is_nullary()) return 0;
    else return arguments_.size();
  }

  const argument_list& arguments() const
  { return arguments_; }
  const argument_list& arguments()
  { return arguments_; }

  void add_argument_type(const std::shared_ptr<type>& th) 
  { arguments_.emplace_back(th); }

  bool equals(const type& o) const override {
    auto of = dynamic_cast<const function_type*>(&o);
    if(!of) return false; // not a function_type
    if(!this->return_type_->equals(*(of->return_type()))) // return_type not equal
      return false;
    if(this->argument_size() != of->argument_size()) // not the same amount of arguments
      return false;
    // in a world without zip_iterator everything is bleak
    for(auto it1 = this->arguments().begin(), it2 = of->arguments().begin();
        it1 != this->arguments().end(); ++it1, ++it2) {
      if(!(*it1)->equals(*(it2->get())))
        return false;
    }
    return true;
  }

  std::ostream& print(std::ostream& os) const override {
    os << "function returning '" << *return_type_ << "' with arguments (\n";
    for(auto& x : arguments_) {
      os << "\t'" << *x << '\'' << '\n'; 
    }
    return os << ')';
  }

  virtual llvm::Type* llvm_type(llvm::IRBuilder<>& b) override {
    auto restype = return_type_->llvm_type(b);
    std::vector<llvm::Type*> args; args.reserve(is_nullary() ? 0 : arguments().size());
    if(!is_nullary())
      for(auto& a : arguments()) {
        args.emplace_back(a->llvm_type(b));
      }
    auto funtype = llvm::FunctionType::get(restype, std::move(args),false);
    return funtype;
  }

private:
  std::shared_ptr<type> return_type_;
  argument_list arguments_;
};

struct error_type : type {
  static std::shared_ptr<error_type> get_error() {
    static std::shared_ptr<error_type> instance(new error_type);
    return instance;
  }
  bool is_error() const override final { return true; }
  bool is_complete() const override { return false;}

  bool equals(const type& o) const override
  { return dynamic_cast<const error_type*>(&o); }

  std::ostream& print(std::ostream& os) const override {
    return os << "error";
  }
private:
  error_type() {};
};

inline arithmetic_type* as_arithmetic_type(type* t) {
  return dynamic_cast<arithmetic_type*>(t);
}
inline scalar_type* as_scalar_type(type* t) {
  return dynamic_cast<scalar_type*>(t);
}
inline object_type* as_object_type(type* t) {
  return dynamic_cast<object_type*>(t);
}
inline void_type* as_void_type(type* t) {
  return dynamic_cast<void_type*>(t);
}
inline struct_type* as_struct_type(type* t) {
  return dynamic_cast<struct_type*>(t);
}
inline function_type* as_function_type(type* t) {
  return dynamic_cast<function_type*>(t);
}
inline pointer_type* as_pointer_type(type* t) {
  return dynamic_cast<pointer_type*>(t);
}
inline error_type* as_error_type(type* t) {
  return dynamic_cast<error_type*>(t);
}

inline std::shared_ptr<arithmetic_type> as_arithmetic_type(const std::shared_ptr<type>& t) {
  return std::dynamic_pointer_cast<arithmetic_type>(t);
}
inline std::shared_ptr<scalar_type> as_scalar_type(const std::shared_ptr<type>& t) {
  return std::dynamic_pointer_cast<scalar_type>(t);
}
inline std::shared_ptr<object_type> as_object_type(const std::shared_ptr<type>& t) {
  return std::dynamic_pointer_cast<object_type>(t);
}
inline std::shared_ptr<void_type> as_void_type(const std::shared_ptr<type>& t) {
  return std::dynamic_pointer_cast<void_type>(t);
}
inline std::shared_ptr<struct_type> as_struct_type(const std::shared_ptr<type>& t) {
  return std::dynamic_pointer_cast<struct_type>(t);
}
inline std::shared_ptr<function_type> as_function_type(const std::shared_ptr<type>& t) {
  return std::dynamic_pointer_cast<function_type>(t);
}
inline std::shared_ptr<pointer_type> as_pointer_type(const std::shared_ptr<type>& t) {
  return std::dynamic_pointer_cast<pointer_type>(t);
}
inline std::shared_ptr<error_type> as_error_type(const std::shared_ptr<type>& t) {
  return std::dynamic_pointer_cast<error_type>(t);
}

}

#endif /* C4_TYPE_H */
