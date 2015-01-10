#ifndef C4_AST_H
#define C4_AST_H

#include "ast_fwd.h"
#include "ast_visitor.h"
#include "token.h"
#include "pos.h"
#include <string>
#include <vector>
#include <memory>

#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"

namespace c4 {

typedef std::map<std::string, llvm::AllocaInst*> named_values_map;

struct type;

enum class linkage {
  NOT_DETERMINED = 0, EXTERNAL, INTERNAL, NONE
};

struct ast_node {
  //! Return the class name of this object.
  ast_node(const ast_node&) = delete;
  ast_node& operator=(const ast_node&) = delete;
  virtual ~ast_node() = default;
  virtual void visit(ast_visitor*) = 0;
  const Pos& position() { return pos_; }
  virtual bool is_stmt() const { return false; }
  virtual bool is_expression() const { return false; }
  virtual bool is_decl() const { return false; }
protected:
  ast_node() : pos_{"BAD POSITION"} {}
  ast_node(const Pos& pos) : pos_(pos) {}
  const Pos pos_;
};

////////// DECLARATIONS //////////

struct translation_unit : ast_node {
  void visit(ast_visitor* a) { a->visit(this); }

  const std::vector<std::unique_ptr<decl>>& decls() const { return decls_; }

  //! Add a declaration to this translation unit.
  void add_decl(decl* d) { decls_.emplace_back(d); }
private:
  std::vector<std::unique_ptr<decl>> decls_;
};

struct type_specifier : ast_node {
  type_specifier(const Pos& p, token_type t) : ast_node(p), token(t) {}

  void visit(ast_visitor* a) { a->visit(this); }
  token_type token;

  bool has_type() const { return type_.get() != 0; }
  std::shared_ptr<type>& get_type() { return type_; }
  void set_type(const std::shared_ptr<type>& t) { type_ = t; }
private:
  std::shared_ptr<type> type_;
};

//! abstract base for all kinds of decls
struct base_decl : ast_node {
  base_decl(const Pos& p, type_specifier* ts, declarator* dl)
    : ast_node{p}, ts_{ts}, dl_{dl}, name_{}, linkage_{linkage::NOT_DETERMINED} {}

  type_specifier* get_type_specifier() { return ts_.get(); }
  declarator* get_declarator() { return dl_.get(); }
  const declarator* get_declarator() const { return dl_.get(); }

  bool has_type() const { return ts_->has_type(); }
  std::shared_ptr<type>& get_type() { return ts_->get_type(); }
  void set_type(const std::shared_ptr<type>& t) { ts_->set_type(t); }

  const std::string& get_name() const;

  const std::vector<std::unique_ptr<parameter_decl>>&
  parameter_decls() const;

  const linkage& get_linkage() const { return linkage_; }
  void set_linkage(const linkage& l) { linkage_ = l; }

  bool is_decl() const override final { return true; }

  llvm::Function* gen_func_code(llvm::Module&, llvm::IRBuilder<>&, 
				llvm::IRBuilder<>&, named_values_map*) const;

private:
  std::unique_ptr<type_specifier> ts_;
  std::unique_ptr<declarator> dl_;
  mutable std::string name_;
  linkage linkage_;
};

//! A decl used for parameters.
struct parameter_decl : base_decl {
  using base_decl::base_decl;
  void visit(ast_visitor* a) { a->visit(this); }
};

//! A base_decl with an optional body.
struct decl : base_decl {
  using base_decl::base_decl;
  void visit(ast_visitor* a) { a->visit(this); }

  void set_body(comp_stmt* s) { s_.reset(s); }
  comp_stmt* get_body() { return s_.get(); }
  bool has_body() const { return s_ != nullptr; }
  void gen_code(llvm::Module&, llvm::IRBuilder<>&, 
		llvm::IRBuilder<>&, named_values_map*) const;
private:
  std::unique_ptr<comp_stmt> s_;
};


struct struct_specifier : type_specifier {
  struct_specifier(const Pos& p, token_type t, const std::string& tag)
    : type_specifier(p, t), tag_(tag) {}
  void visit(ast_visitor* a) { a->visit(this); }
  bool has_decls() const { return !decls_.empty(); }
  const std::string& get_tag() const { return tag_; }
  void add_decl(decl* d) { decls_.emplace_back(d); }
  const std::vector<std::unique_ptr<decl>>& decls() const { return decls_; }
private:
  std::vector<std::unique_ptr<decl>> decls_;
  const std::string tag_;
};

struct declarator : ast_node {
  declarator(bool p, declarator* d) : p_(p), d_(d) {}
  declarator(bool p, const std::string& identifier)
    : p_(p), d_(nullptr), ident_(identifier) {}

  void visit(ast_visitor* a) { a->visit(this); }

  bool pointer() { return p_; }
  declarator* get_declarator() const { return d_.get(); }

  const std::string& get_identifier() const { return ident_; }

  const std::vector<std::unique_ptr<parameter_decl>>&
  parameter_decls() const { return pds_; }

  void add_parameter_decl(parameter_decl* pd) { pds_.emplace_back(pd); }
private:
  bool p_;
  std::unique_ptr<declarator> d_;
  std::vector<std::unique_ptr<parameter_decl>> pds_;
  std::string ident_;
};

struct type_name : base_decl {
  using base_decl::base_decl;
  void visit(ast_visitor* a) { a->visit(this); }
};

////////// STATEMENTS //////////

struct stmt : ast_node {
  void visit(ast_visitor* a) { a->visit(this); }
  virtual void gen_code(llvm::Module&, llvm::IRBuilder<>&, 
			llvm::IRBuilder<>&, named_values_map&)=0;

  bool is_stmt() const override final { return true; }
protected:
  using ast_node::ast_node;
};

struct comp_stmt : stmt {
  void add_stmt(ast_node* s) { stmts_.emplace_back(s); }
  const std::vector<std::unique_ptr<ast_node>>& sub_stmts() const {
    return stmts_;
  }
  void visit(ast_visitor* a) { a->visit(this); }
  void gen_code(llvm::Module&, llvm::IRBuilder<>& builder,
		llvm::IRBuilder<>&, named_values_map&) override;
private:
  std::vector<std::unique_ptr<ast_node>> stmts_;
};

struct break_stmt : stmt {
  break_stmt(const Pos& pos) : stmt(pos) {}
  void visit(ast_visitor* a) { a->visit(this); }
  void gen_code(llvm::Module&, llvm::IRBuilder<>& builder, 
		llvm::IRBuilder<>&, named_values_map&) override;
};

struct continue_stmt : stmt {
  continue_stmt(const Pos& pos) : stmt(pos) {}
  void visit(ast_visitor* a) { a->visit(this); }
  void gen_code(llvm::Module&, llvm::IRBuilder<>& builder, 
		llvm::IRBuilder<>&, named_values_map&) override;
};

struct return_stmt : stmt {
  return_stmt(expression* expr, const Pos& pos) : stmt(pos), expr_(expr) {}
  return_stmt(const Pos& pos) : stmt(pos), expr_(nullptr) {}
  void visit(ast_visitor* a) { a->visit(this); }
  expression* expr() { return expr_.get(); }
  void gen_code(llvm::Module&, llvm::IRBuilder<>& builder, 
		llvm::IRBuilder<>&, named_values_map&) override;
  std::shared_ptr<type>& exp_rtype() { return type_; }
private:
  std::shared_ptr<type> type_;
  std::unique_ptr<expression> expr_;
};

struct labeled_stmt : stmt {
 labeled_stmt(const std::string& label, stmt* state, const Pos& pos)
   : stmt(pos), block_(nullptr), label_(label), stmt_(state) {}
  void visit(ast_visitor* a) { a->visit(this); }
  const std::string& get_label() { return label_; }
  stmt* get_stmt() { return stmt_.get(); }
  void gen_code(llvm::Module&, llvm::IRBuilder<>& builder, 
		llvm::IRBuilder<>&, named_values_map&) override;
  llvm::BasicBlock* get_block(llvm::IRBuilder<>&);
private:
  llvm::BasicBlock* block_;
  const std::string label_;
  std::unique_ptr<stmt> stmt_;
};

struct goto_stmt : stmt {
  goto_stmt(const std::string& ident, const Pos& pos)
    : stmt(pos), ident_(ident) {}
  void visit(ast_visitor* a) { a->visit(this); }
  const std::string& get_label() { return ident_; }
  void set_label(labeled_stmt* stmt) { stmt_ = stmt; }
  void gen_code(llvm::Module&, llvm::IRBuilder<>& builder, 
		llvm::IRBuilder<>&, named_values_map&) override;
private:
  labeled_stmt* stmt_;
  const std::string ident_;
};

struct while_stmt : stmt {
 while_stmt(expression* cond, stmt* body, const Pos& pos)
   : stmt(pos), cond_(cond), body_(body) {}
  void visit(ast_visitor* a) { a->visit(this); }
  expression* condition() { return cond_.get(); }
  stmt* body() { return body_.get(); }
  void gen_code(llvm::Module&, llvm::IRBuilder<>& builder, 
		llvm::IRBuilder<>&, named_values_map&) override;
private:
  std::unique_ptr<expression> cond_;
  std::unique_ptr<stmt> body_;
};

struct if_stmt : stmt {
  if_stmt(expression* cond, stmt* body, const Pos& pos)
    : stmt(pos), cond_(cond), body_(body) {}
  stmt* body() { return body_.get(); }
  expression* condition() { return cond_.get(); }
  void gen_code(llvm::Module&, llvm::IRBuilder<>& builder, 
		llvm::IRBuilder<>&, named_values_map&) override;
  void visit(ast_visitor* a) { a->visit(this); }
private:
  std::unique_ptr<expression> cond_;
  std::unique_ptr<stmt> body_;
};

struct if_else_stmt : stmt {
  if_else_stmt(expression* cond, stmt* if_body, stmt* else_body, const Pos& pos)
    : stmt(pos), cond_(cond), if_body_(if_body), else_body_(else_body) {}
  void visit(ast_visitor* a) { a->visit(this); }
  expression* condition() { return cond_.get(); }
  stmt* if_body() { return if_body_.get(); }
  stmt* else_body() { return else_body_.get(); }
  void gen_code(llvm::Module&, llvm::IRBuilder<>& builder, 
		llvm::IRBuilder<>&, named_values_map&) override;
private:
  std::unique_ptr<expression> cond_;
  std::unique_ptr<stmt> if_body_;
  std::unique_ptr<stmt> else_body_;
};

struct expr_stmt : stmt {
  expr_stmt(expression* expr) : expr_(expr) {}
  expr_stmt() : expr_(nullptr) {}
  void visit(ast_visitor* a) { a->visit(this); }
  expression* expr() { return expr_.get(); }
  void gen_code(llvm::Module&, llvm::IRBuilder<>& builder, 
		llvm::IRBuilder<>&, named_values_map&) override;
private:
  std::unique_ptr<expression> expr_;
};

////////// EXPRESSIONS  //////////

struct expression : ast_node {
  void visit(ast_visitor* a) { a->visit(this); }
  std::shared_ptr<type>& e_type() { return type_; }
  virtual llvm::Value* lvalue(llvm::Module&,
			      llvm::IRBuilder<>&,
                              named_values_map&) 
    const { assert(false); return nullptr; }
  virtual llvm::Value* rvalue(llvm::Module&,
			      llvm::IRBuilder<>&,
                              named_values_map&) 
    const=0;
  virtual llvm::Value* cfvalue(llvm::Module& m,
			       llvm::IRBuilder<>& builder,
			       named_values_map& named_values) const { 
    auto val = rvalue(m, builder, named_values);
    auto zero = llvm::Constant::getNullValue(val->getType());
    return builder.CreateICmpNE(val, zero);
  }

  bool is_expression() const override final { return true; }
protected:
  std::shared_ptr<type> type_;
  using ast_node::ast_node;
};

struct primary_expression : expression {
  primary_expression(const token& val, const Pos& pos)
    : expression(pos), val_(val) {}
  void visit(ast_visitor* a) { a->visit(this); }
  const token& value() const { return val_; }
  llvm::Value* lvalue(llvm::Module&, llvm::IRBuilder<>&, named_values_map&) 
    const override;
  llvm::Value* rvalue(llvm::Module&, llvm::IRBuilder<>&, named_values_map&) 
    const override;
  std::string purify_str() const;
private:
  const token val_;
};

struct sizeof_expr : expression {
  sizeof_expr(expression* expr, const Pos& pos) : expression(pos), expr_(expr) {}
  void visit(ast_visitor* a) { a->visit(this); }
  expression* expr() { return expr_.get(); }
  llvm::Value* rvalue(llvm::Module&, llvm::IRBuilder<>&, named_values_map&) 
    const override;
private:
  std::unique_ptr<expression> expr_;
};

struct sizeof_type : expression {
  sizeof_type(type_name* tn, const Pos& pos) : expression(pos), tn_(tn) {}
  void visit(ast_visitor* a) { a->visit(this); }
  type_name* get_type_name() { return tn_.get(); }
  llvm::Value* rvalue(llvm::Module&, llvm::IRBuilder<>&, named_values_map&) 
    const override;
private:
  std::unique_ptr<type_name> tn_;
};

struct unary_operator : expression {
  unary_operator(token_type op, expression* operand, const Pos& pos)
    : expression(pos), operator_(op), operand_(operand) {}
  void visit(ast_visitor* a) { a->visit(this); }
  token_type op() { return operator_; }
  expression* operand() { return operand_.get(); }
  llvm::Value* lvalue(llvm::Module&, llvm::IRBuilder<>&, named_values_map&) 
    const override;
  llvm::Value* rvalue(llvm::Module&, llvm::IRBuilder<>&, named_values_map&) 
    const override;
  llvm::Value* cfvalue(llvm::Module&, llvm::IRBuilder<>&, named_values_map&)
    const override;
private:
  token_type operator_;
  std::unique_ptr<expression> operand_;
};

struct binary_operator : expression {
  binary_operator(token_type op, expression* left, expression* right,
                  const Pos& pos)
    : expression(pos), op_(op), left_(left), right_(right) {}
  void visit(ast_visitor* a) { a->visit(this); }
  token_type op() { return op_; }
  expression* left() { return left_.get(); }
  expression* right() { return right_.get(); }
  llvm::Value* rvalue(llvm::Module&, llvm::IRBuilder<>&, named_values_map&) 
    const override;
  llvm::Value* cfvalue(llvm::Module&, llvm::IRBuilder<>&, named_values_map&)
    const override;
private:
  inline bool is_arithmetic() const { 
    return op_ == token_type::PCTR_PLUS || op_ == token_type::PCTR_STAR ||
      op_ == token_type::PCTR_MINUS || op_ == token_type::PCTR_ASSIGN;
  }
  inline bool is_logical() const { 
    return op_ == token_type::PCTR_AND || op_ == token_type::PCTR_OR;
  }
  token_type op_;
  std::unique_ptr<expression> left_;
  std::unique_ptr<expression> right_;
};

struct postfix_operator : expression {
  postfix_operator(token_type op, expression* left, expression* right,
                   const Pos& pos)
    : expression(pos), op_(op), left_(left), right_(right) {}
  void visit(ast_visitor* a) { a->visit(this); }
  token_type op() { return op_; }
  expression* left() { return left_.get(); }
  expression* right() { return right_.get(); }
  llvm::Value* rvalue(llvm::Module&, llvm::IRBuilder<>&, named_values_map&) 
    const override;
  llvm::Value* lvalue(llvm::Module&, llvm::IRBuilder<>&, named_values_map&) 
    const override;
private:
  token_type op_;
  std::unique_ptr<expression> left_;
  std::unique_ptr<expression> right_;
};

struct subscript_operator : expression {
  subscript_operator(expression* left, expression* right, const Pos& pos)
    : expression(pos), left_(left), right_(right) {}
  void visit(ast_visitor* a) { a->visit(this); }
  expression* left() { return left_.get(); }
  expression* right() { return right_.get(); }
  llvm::Value* rvalue(llvm::Module&, llvm::IRBuilder<>&, named_values_map&) 
    const override;
  llvm::Value* lvalue(llvm::Module&, llvm::IRBuilder<>&, named_values_map&) 
    const override;
private:
  std::unique_ptr<expression> left_;
  std::unique_ptr<expression> right_;
};

struct function_call : expression {
  function_call(expression* func, const Pos& pos)
    : expression(pos), func_(func) {}
  void visit(ast_visitor* a) { a->visit(this); }
  void add_param(expression* p) { params_.emplace_back(p); }
  const std::vector<std::unique_ptr<expression>>& params() const {
    return params_;
  }
  expression* get_name() { return func_.get(); }
  llvm::Value* rvalue(llvm::Module&, llvm::IRBuilder<>&, named_values_map&) 
    const override;
private:
  std::unique_ptr<expression> func_;
  std::vector<std::unique_ptr<expression>> params_;
};

struct ternary_expr : expression {
  ternary_expr(expression* test, expression* true_expr,
	       expression* false_expr, const Pos& pos)
    : expression(pos), test_(test), true_(true_expr), false_(false_expr) {}
  void visit(ast_visitor* a) { a->visit(this); }
  expression* test() { return test_.get(); }
  expression* true_expr() { return true_.get(); }
  expression* false_expr() { return false_.get(); }
  llvm::Value* rvalue(llvm::Module&, llvm::IRBuilder<>&, named_values_map&) 
    const override;
private:
  std::unique_ptr<expression> test_;
  std::unique_ptr<expression> true_;
  std::unique_ptr<expression> false_;
};

} // c4

#endif /* C4_AST_H */
