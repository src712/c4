#include "ast.h"
#include "type.h"
#include "compile.h"

#include "llvm/IR/ValueSymbolTable.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Analysis/Verifier.h"

namespace {
llvm::BasicBlock* create_block(llvm::IRBuilder<>&);
void gen_cond_branch(llvm::Module& m, llvm::IRBuilder<>& builder, 
		     llvm::IRBuilder<>& alloca_builder, 
		     c4::named_values_map& named_values,
		     const std::unique_ptr<c4::expression>& cond,
		     std::unique_ptr<c4::stmt>& body,
		     llvm::BasicBlock* false_blk,
		     llvm::BasicBlock* end_blk);
llvm::Value* type_size(const std::shared_ptr<c4::type> type, 
			 llvm::IRBuilder<>& builder);
llvm::Value*
ptr_addition(llvm::Module& m, 
	     llvm::IRBuilder<>& builder,
	     c4::named_values_map& named_values, 
	     const std::unique_ptr<c4::expression>& left,
	     const std::unique_ptr<c4::expression>& right,
	     bool sub=0);
llvm::Value* rvalue_as_int(const std::unique_ptr<c4::expression>&,
			   llvm::Module&,  
			   llvm::IRBuilder<>&,
			   c4::named_values_map&);
llvm::Value* rvalue_as_char(const std::unique_ptr<c4::expression>&,
			    llvm::Module&,  
			    llvm::IRBuilder<>&, 
			    c4::named_values_map&);
llvm::Value* as_int(llvm::Value*, llvm::IRBuilder<>&);
llvm::Value* cast_val(const std::unique_ptr<c4::expression>&, 
		      const std::shared_ptr<c4::type>&,
		      llvm::Type*, 
		      llvm::Module&,  
		      llvm::IRBuilder<>&, 
		      c4::named_values_map&);
std::vector<std::pair<llvm::BasicBlock*, llvm::BasicBlock*>> loops;
bool jumped = false;
}

const std::string& c4::base_decl::get_name() const {
  // We use name_.empty() to designate "not yet retrieved". This sucks
  // in the case of abstract decls, but hey, we are not supposed to
  // call it that often.
  if(name_.empty()) {
    auto dl = get_declarator();
    if(!dl) return name_;

    do {
      if(!dl->get_identifier().empty())
        return name_ = dl->get_identifier();
    } while((dl = dl->get_declarator()));
  }
  return name_;
}

const std::vector<std::unique_ptr<c4::parameter_decl>>&
c4::base_decl::parameter_decls() const {
  // search through the declarators to find our parameters
  auto dl = get_declarator();
  assert(dl); // never call that on something that is not a function
  do {
    if(!dl->parameter_decls().empty())
      return dl->parameter_decls();
  } while((dl = dl->get_declarator()));
  assert(false);
  return dl->parameter_decls(); // never get here
}

llvm::Function*
c4::base_decl::gen_func_code(llvm::Module& m, llvm::IRBuilder<>& builder, 
			     llvm::IRBuilder<>& alloca_builder,
			     named_values_map* named_values) const {
  using namespace llvm;
  auto type = ts_->get_type();
  auto ltype = type->llvm_type(builder);

  if((ltype->isIntegerTy() || ltype->isPointerTy() || ltype->isStructTy())
     && !get_name().empty()) {
    if(get_linkage() != c4::linkage::EXTERNAL) { // file local
      auto var = alloca_builder.CreateAlloca(ltype, 0, get_name());
      (*named_values)[get_name()] = var;
    } else {
      new GlobalVariable(
	m, ltype, false, GlobalValue::CommonLinkage, // todo right linkage??
        Constant::getNullValue(ltype), get_name());
    }
  } else if(ltype->isFunctionTy()) {
    auto func = Function::Create(cast<FunctionType>(ltype), 
				 GlobalValue::ExternalLinkage, get_name(), &m);
    auto it = func->arg_begin();
    for(auto& p : parameter_decls()) {
      it->setName(p->get_name());
      ++it;
    }
    return func;
  }
  return nullptr;
}

void
c4::decl::gen_code(llvm::Module& m, llvm::IRBuilder<>& builder, 
		   llvm::IRBuilder<>& alloca_builder,
		   named_values_map* old_named_values) const {
  using namespace llvm;
  auto fun = gen_func_code(m, builder, alloca_builder, old_named_values);
  if(fun && fun->getName() != get_name()) {
    fun->eraseFromParent();
    fun = m.getFunction(get_name());
  }
  
  if(fun && has_body()) {
    auto block = BasicBlock::Create(
      builder.getContext(), "entry", fun, 0); // always named entry
    builder.SetInsertPoint(block);
    alloca_builder.SetInsertPoint(block);

    std::map<std::string, llvm::AllocaInst*> new_named_values;

    // generate stack space for the function arguments
    for(auto it = fun->arg_begin(); it != fun->arg_end(); ++it) {
      auto arg = alloca_builder.CreateAlloca(it->getType(), 0);
      new_named_values[it->getName()] = arg; // no checks, we know this is safe.
      builder.CreateStore(it, arg);
    }
    
    // generate code for the function body (comp_stmt)
    s_->gen_code(m, builder, alloca_builder, new_named_values);
  
    // make sure function has a terminator
    if(!builder.GetInsertBlock()->getTerminator()) {
    Type *cur_ret_type = builder.getCurrentFunctionReturnType();
      if (cur_ret_type->isVoidTy())
	builder.CreateRetVoid();
      else
	builder.CreateRet(Constant::getNullValue(cur_ret_type));
    }
    jumped = false;
    verifyFunction(*fun);
  }
}

void c4::comp_stmt::gen_code(llvm::Module& m, llvm::IRBuilder<>& builder, 
			     llvm::IRBuilder<>& alloca_builder,
			     named_values_map& named_values) {
  for(auto& ss : stmts_) {
    if(jumped) break;
    if(ss->is_stmt()) {
      auto stmt = dynamic_cast<c4::stmt*>(ss.get());
      assert(stmt);
      stmt->gen_code(m, builder, alloca_builder, named_values);
    } else if(ss->is_decl()) {
      auto dd = dynamic_cast<c4::decl*>(ss.get());
      assert(dd);
      dd->gen_code(m, builder, alloca_builder, &named_values);
    }
  }
}

void c4::break_stmt::gen_code(llvm::Module&, llvm::IRBuilder<>& builder, 
			  llvm::IRBuilder<>&, named_values_map&) {
  jumped = true;
  builder.CreateBr((loops.end() - 1)->second);
}

void c4::continue_stmt::gen_code(llvm::Module&, llvm::IRBuilder<>& builder, 
			  llvm::IRBuilder<>&, named_values_map&) {
  jumped = true;
  builder.CreateBr((loops.end() - 1)->first);
}

void c4::return_stmt::gen_code(llvm::Module& m, llvm::IRBuilder<>& builder, 
			       llvm::IRBuilder<>&,
                               named_values_map& named_values) {
  auto ret_type = builder.getCurrentFunctionReturnType();
  if(!expr_)
    builder.CreateRetVoid();
  else if(ret_type->isIntegerTy(32))
    builder.CreateRet(rvalue_as_int(expr_, m, builder, named_values));
  else if(ret_type->isIntegerTy(8))
    builder.CreateRet(rvalue_as_char(expr_, m, builder, named_values));
  else if(expr_->e_type()->is_zero())
    builder.CreateRet(llvm::Constant::getNullValue(ret_type));
  else
    builder.CreateRet(cast_val(expr_, exp_rtype(), ret_type, 
			       m, builder, named_values));
  jumped = true;
}

void c4::labeled_stmt::gen_code(llvm::Module& m, llvm::IRBuilder<>& builder, 
				llvm::IRBuilder<>& alloca_builder, 
				named_values_map& named_values) {
  builder.CreateBr(get_block(builder));
  builder.SetInsertPoint(get_block(builder));
  stmt_->gen_code(m, builder, alloca_builder, named_values);
}

llvm::BasicBlock* c4::labeled_stmt::get_block(llvm::IRBuilder<>& builder) {
  if(!block_)
    block_ = create_block(builder);
  return block_;
}

void c4::goto_stmt::gen_code(llvm::Module&, llvm::IRBuilder<>& builder, 
			     llvm::IRBuilder<>&, named_values_map&) {
  builder.CreateBr(stmt_->get_block(builder));
  builder.SetInsertPoint(create_block(builder));
}

namespace {
void gen_cond_branch(llvm::Module& m, llvm::IRBuilder<>& builder, 
		     llvm::IRBuilder<>& alloca_builder, 
		     c4::named_values_map& named_values,
		     const std::unique_ptr<c4::expression>& cond,
		     std::unique_ptr<c4::stmt>& body,
		     llvm::BasicBlock* false_blk,
		     llvm::BasicBlock* end_blk) {
  using namespace llvm;
  Value* cond_val = cond->cfvalue(m, builder, named_values);
  BasicBlock* body_blk = create_block(builder);
  builder.CreateCondBr(cond_val, body_blk, false_blk);
  builder.SetInsertPoint(body_blk);
  body->gen_code(m, builder, alloca_builder, named_values);
  if(!jumped)
    builder.CreateBr(end_blk);
  jumped = false;
  builder.SetInsertPoint(false_blk);
}
}

void c4::while_stmt::gen_code(llvm::Module& m, llvm::IRBuilder<>& builder, 
			      llvm::IRBuilder<>& alloca_builder, 
			      named_values_map& named_values) {
  using namespace llvm;
  BasicBlock* header = create_block(builder);
  BasicBlock* end = create_block(builder);
  loops.emplace_back(std::make_pair(header, end));
  builder.CreateBr(header);
  builder.SetInsertPoint(header);
  gen_cond_branch(m, builder, alloca_builder, 
		  named_values, cond_, body_, end, header);
  loops.pop_back();
}

void c4::if_stmt::gen_code(llvm::Module& m, llvm::IRBuilder<>& builder, 
			   llvm::IRBuilder<>& alloca_builder, 
			   named_values_map& named_values) {
  using namespace llvm;
  BasicBlock* end = create_block(builder);
  gen_cond_branch(m, builder, alloca_builder, 
		  named_values, cond_, body_, end, end);
}

void c4::if_else_stmt::gen_code(llvm::Module& m, llvm::IRBuilder<>& builder, 
				llvm::IRBuilder<>& alloca_builder, 
				named_values_map& named_values) {
  using namespace llvm;
  BasicBlock* else_body = create_block(builder);
  BasicBlock* end = create_block(builder);
  gen_cond_branch(m, builder, alloca_builder, 
		  named_values, cond_, if_body_, else_body, end);
  else_body_->gen_code(m, builder, alloca_builder, named_values);
  if(!jumped)
    builder.CreateBr(end);
  jumped = false;
  builder.SetInsertPoint(end);
}

void c4::expr_stmt::gen_code(llvm::Module& m, llvm::IRBuilder<>& builder, 
			     llvm::IRBuilder<>&,
			     named_values_map& named_values) {
  if(expr_)
    expr_->rvalue(m, builder, named_values);
}


llvm::Value* 
c4::primary_expression::lvalue(llvm::Module& m, 
			       llvm::IRBuilder<>&, 
			       named_values_map& named_values) const {
  assert(val_.type == c4::token_type::IDENTIFIER);
  auto it = named_values.find(val_.data);
  if(it != named_values.end())
    return it->second;
  else // function cannot be an lvalue
    return m.getGlobalVariable(val_.data);
}

llvm::Value* 
c4::primary_expression::rvalue(llvm::Module& m, 
			       llvm::IRBuilder<>& builder, 
			       named_values_map& named_values) const { 
  switch(val_.type) {
  case token_type::INTEGER_CONSTANT:
    return builder.getInt32(std::stoi(val_.data));
  case token_type::CHARACTER_CONSTANT:
    return builder.getInt32(purify_str().c_str()[0]);
  case token_type::STRING_LITERAL:
    return builder.CreateGlobalStringPtr(purify_str());
  default: {  // identifier
    if(auto func = m.getFunction(val_.data))
      return func;
    return builder.CreateLoad(lvalue(m, builder, named_values));
  }
  }
}

std::string c4::primary_expression::purify_str() const {
  assert(val_.data.size() >= 2);
  std::string s;
  s.reserve(val_.data.size());
  for(unsigned int i = 1; i < val_.data.size() - 1; ++i) {
    if(val_.data[i] != '\\') {
      s += val_.data[i];
    } else {
      ++i;
      switch(val_.data[i]) {
      case '\'': s += '\''; break;
      case '\"': s += '\"'; break;
      case '?': s += '\?'; break;
      case '\\': s += '\\'; break;
      case 'a': s += '\a'; break;
      case 'b': s += '\b'; break;
      case 'f': s += '\f'; break;
      case 'n': s += '\n'; break;
      case 'r': s += '\r'; break;
      case 't': s += '\t'; break;
      default: s += '\v';  
      }
    }
  }
  return s;
}

llvm::Value* 
c4::sizeof_expr::rvalue(llvm::Module&, llvm::IRBuilder<>& builder, 
			named_values_map&) const {
  if(auto pe = dynamic_cast<const primary_expression*> (expr_.get())) {
    if(pe->value().type == token_type::STRING_LITERAL)
      return builder.getInt32(pe->purify_str().size() + 1);
  }
  return type_size(expr_->e_type(), builder);
}

llvm::Value* 
c4::sizeof_type::rvalue(llvm::Module&, llvm::IRBuilder<>& builder, 
			named_values_map&) const { 
  return type_size(tn_->get_type(), builder);
}

llvm::Value* 
c4::unary_operator::lvalue(llvm::Module& m, 
			    llvm::IRBuilder<>& builder, 
			    named_values_map& named_values) const {
  assert(operator_ == token_type::PCTR_STAR);
  return operand_->rvalue(m, builder, named_values);
}

llvm::Value* 
c4::unary_operator::rvalue(llvm::Module& m, 
			    llvm::IRBuilder<>& builder, 
			    named_values_map& named_values) const {
  switch(operator_) {
  case token_type::PCTR_STAR:
    return builder.CreateLoad(operand_->rvalue(m, builder, named_values));
  case token_type::PCTR_BANG:
    return as_int(cfvalue(m, builder, named_values), builder);
  case token_type::PCTR_MINUS: {
    auto operand = rvalue_as_int(operand_, m, builder, named_values);
    return builder.CreateSub(builder.getInt32(0), operand);
  }
  default: // PCTR_BIT_AND
    return operand_->lvalue(m, builder, named_values);
  }
}

llvm::Value* 
c4::unary_operator::cfvalue(llvm::Module& m, 
			    llvm::IRBuilder<>& builder, 
			    named_values_map& named_values) const {
  if(operator_ == token_type::PCTR_BANG) {
    auto val = operand_->rvalue(m, builder, named_values);
    auto zero = llvm::Constant::getNullValue(val->getType());
    return builder.CreateICmpEQ(val, zero);
  } else 
    return expression::cfvalue(m, builder, named_values);
}

llvm::Value* 
c4::binary_operator::rvalue(llvm::Module& m, 
			    llvm::IRBuilder<>& builder, 
			    named_values_map& named_values) const {
  using namespace llvm;
  if(op_ == token_type::PCTR_ASSIGN) {
    auto left = left_->lvalue(m, builder, named_values);
    llvm::Value* right;
    if(type_->is_char())
      right = rvalue_as_char(right_, m, builder, named_values);
    else if(type_->is_int())
      right = rvalue_as_int(right_, m, builder, named_values);
    else if(right_->e_type()->is_zero())
      right = Constant::getNullValue(left->getType());
    else
      right = cast_val(right_, left_->e_type(), left->getType(), 
		       m, builder, named_values);
    builder.CreateStore(right, left);
    return right;
  } else if(!is_arithmetic()) {
    return as_int(cfvalue(m, builder, named_values), builder);
  } // op_ is +, -, or *

  if(type_->is_pointer()) // pointer +/- int 
    return ptr_addition(m, builder, named_values, left_, right_, 
			op_ == token_type::PCTR_MINUS);


  auto left = rvalue_as_int(left_, m, builder, named_values);
  auto right = rvalue_as_int(right_, m, builder, named_values);

  switch(op_) {
  case token_type::PCTR_PLUS:
    return builder.CreateAdd(left, right);
  case token_type::PCTR_STAR:
    return builder.CreateMul(left, right);
  default: // MINUS
    return builder.CreateSub(left, right);
  }
}

llvm::Value* 
c4::binary_operator::cfvalue(llvm::Module& m, 
			     llvm::IRBuilder<>& builder, 
			     named_values_map& named_values) const {
  using namespace llvm;
  if(is_arithmetic()) {
    return expression::cfvalue(m, builder, named_values);
  } else if(is_logical()) { 
    bool and_op = op_ == token_type::PCTR_AND;
    BasicBlock* curr = builder.GetInsertBlock();
    BasicBlock* t_block = create_block(builder);
    BasicBlock* f_block = create_block(builder);
    auto l_cond = left_->cfvalue(m, builder, named_values);
    builder.CreateCondBr(l_cond, t_block, f_block);
    auto first = and_op ? t_block : f_block;
    auto second = and_op ? f_block : t_block;
    builder.SetInsertPoint(first);
    auto r_cond = right_->cfvalue(m, builder, named_values);
    builder.CreateBr(second);
    builder.SetInsertPoint(second);
    PHINode* phi = builder.CreatePHI(builder.getInt1Ty(), 2);
    phi->addIncoming(and_op ? builder.getFalse() : builder.getTrue(), curr);
    phi->addIncoming(r_cond, first);
    return phi;    
  }
 
  auto left = left_->rvalue(m, builder, named_values);
  auto right = right_->rvalue(m, builder, named_values);
   if(left_->e_type()->is_arithmetic() && right_->e_type()->is_arithmetic()) {
    left = as_int(left, builder);
    right = as_int(right, builder);
  } else if(left_->e_type()->is_zero()) {
    left = Constant::getNullValue(right->getType());
  } else if(right_->e_type()->is_zero()) {
    right = Constant::getNullValue(left->getType());
  } else {
    right = builder.CreateBitCast(right, left->getType());
  } 
  
  switch(op_) {
  case token_type::PCTR_EQUAL:
   return builder.CreateICmpEQ(left, right);
  case token_type::PCTR_NOT_EQUAL:
    return builder.CreateICmpNE(left, right);
  default:
    if(left_->e_type()->is_arithmetic())
      return builder.CreateICmpSLT(left, right);
    else
      return builder.CreateICmpULT(left, right);
  }
}

llvm::Value*
c4::postfix_operator::lvalue(llvm::Module& m, llvm::IRBuilder<>& builder, 
			    named_values_map& named_values) const {
  
  bool deref = op_ == token_type::PCTR_DEREF;
  llvm::Value* struct_val = deref ? 
    left_->rvalue(m, builder, named_values) :
    left_->lvalue(m, builder, named_values);
  auto& member = dynamic_cast<primary_expression*> (right_.get())->value().data;
  std::shared_ptr<c4::struct_type> struct_ty;
  if(deref) {
    auto ptr_ty = as_pointer_type(left_->e_type());
    assert(ptr_ty);
    struct_ty = as_struct_type(ptr_ty->underlying());
  } else {
    struct_ty = as_struct_type(left_->e_type());
  }
  assert(struct_ty);
  int index = struct_ty->index_of(member);
  std::vector<llvm::Value*> elm_indexes{builder.getInt32(0), 
      builder.getInt32(index)};
  return builder.CreateInBoundsGEP(struct_val, elm_indexes); 
  
}

llvm::Value*
c4::postfix_operator::rvalue(llvm::Module& m, llvm::IRBuilder<>& builder, 
			    named_values_map& named_values) const {
  return builder.CreateLoad(lvalue(m, builder, named_values));
}

llvm::Value*
c4::subscript_operator::lvalue(llvm::Module& m, llvm::IRBuilder<>& builder, 
			    named_values_map& named_values) const {
  return ptr_addition(m, builder, named_values, left_, right_);
}

llvm::Value*
c4::subscript_operator::rvalue(llvm::Module& m, llvm::IRBuilder<>& builder, 
			    named_values_map& named_values) const {
  return builder.CreateLoad(lvalue(m, builder, named_values));
}

llvm::Value*
c4::function_call::rvalue(llvm::Module& m, llvm::IRBuilder<>& builder, 
			    named_values_map& named_values) const {
  auto func_val = func_->rvalue(m, builder, named_values); 
  if(params_.size() == 0) 
    return builder.CreateCall(func_val);
  std::vector<llvm::Value*> args;
  for(auto& expr : params_)
    args.emplace_back(expr->rvalue(m, builder, named_values));
  return builder.CreateCall(func_val, args);
}

llvm::Value* // TODO do we need to convert any types here?
c4::ternary_expr::rvalue(llvm::Module& m, llvm::IRBuilder<>& builder, 
			    named_values_map& named_values) const {
  return builder.CreateSelect(test_->cfvalue(m, builder, named_values),
			      true_->rvalue(m, builder, named_values),
			      false_->rvalue(m, builder, named_values));
}

namespace {
llvm::BasicBlock* create_block(llvm::IRBuilder<>& builder) {
  llvm::Function* func = builder.GetInsertBlock()->getParent();
  return llvm::BasicBlock::Create(builder.getContext(), "", func);
}

llvm::Value* type_size(const std::shared_ptr<c4::type> type, 
		       llvm::IRBuilder<>& builder) {
  return  builder.CreateTrunc(llvm::ConstantExpr::getSizeOf
			      (type->llvm_type(builder)), builder.getInt32Ty());
}

llvm::Value*
ptr_addition(llvm::Module& m, 
	     llvm::IRBuilder<>& builder,
	     c4::named_values_map& named_values, 
	     const std::unique_ptr<c4::expression>& left,
	     const std::unique_ptr<c4::expression>& right,
	     bool sub) {
  auto& ptr = left->e_type()->is_int() ? right : left;
  auto& index = left->e_type()->is_int() ? left : right;
  auto ptr_val = ptr->rvalue(m, builder, named_values);
  auto index_val = rvalue_as_int(index, m, builder, named_values);
  if(sub)
    index_val = builder.CreateSub(builder.getInt32(0), index_val);
  return builder.CreateInBoundsGEP(ptr_val, index_val); 
  
}

llvm::Value* rvalue_as_int(const std::unique_ptr<c4::expression>& expr,
			   llvm::Module& m,  
			   llvm::IRBuilder<>& builder,
			   c4::named_values_map& named_values) {
  auto right = expr->rvalue(m, builder, named_values);
  if(expr->e_type()->is_arithmetic())
     right = as_int(right, builder);
  else if(expr->e_type()->is_pointer())
    right = builder.CreatePtrToInt(right, builder.getInt32Ty());
  return right;
}

llvm::Value* rvalue_as_char(const std::unique_ptr<c4::expression>& expr,
			    llvm::Module& m, 
			    llvm::IRBuilder<>& builder,
			    c4::named_values_map& named_values) {
  auto right = expr->rvalue(m, builder, named_values);
  if(expr->e_type()->is_int())
     right = builder.CreateTrunc(right, builder.getInt8Ty());
  return right;
}

llvm::Value* as_int(llvm::Value* input, llvm::IRBuilder<>& builder) {
  auto val_type = input->getType();
  if(val_type->isIntegerTy(1))
    return builder.CreateZExt(input, builder.getInt32Ty());
  else
    return builder.CreateSExt(input, builder.getInt32Ty());
}

llvm::Value* cast_val(const std::unique_ptr<c4::expression>& expr,
		      const std::shared_ptr<c4::type>& c4_type,
		      llvm::Type* type, 
		      llvm::Module& m,  
		      llvm::IRBuilder<>& builder, 
		      c4::named_values_map& named_values) {
  auto val = expr->rvalue(m, builder, named_values);
  if(*c4_type != *(expr->e_type()))
    val = builder.CreateBitCast(val, type);
  return val;
}

}
