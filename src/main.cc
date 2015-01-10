#include <stdexcept>
#include <iostream>
#include <algorithm>

#include "llvm/Config/config.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/LLVMContext.h"
#include "diagnostic.h"
#include "input.h"
#include "util.h"
#include "lexer.h"
#include "parser.h"
#include "compile.h"
#include "ast.h"
#include "print_visitor.h"
#include "sema_visitor.h"
#include "optimize.h"

#if LLVM_VERSION_MAJOR != 3
#error "Wrong LLVM Major Version"
#endif

#if LLVM_VERSION_MINOR != 3
#error "Wrong LLVM Minor Version"
#endif

enum class Mode {
  TOKENIZE,
  PARSE,
  PRINT_AST,
  COMPILE,
  OPTIMIZE
};

static void tokenize(c4::lexer& l);
static std::unique_ptr<c4::ast_node> parse(c4::lexer& l);
static void print_ast(c4::lexer& l);
static std::unique_ptr<llvm::Module> build_module(c4::lexer& l, const char* name);
static void write_module(const llvm::Module& m, const char* name);
static void optimize(llvm::Module& m);

int main(int, char** const argv)
{
  try {
    char** i = argv + 1;

    Mode mode = Mode::COMPILE;
    for (; auto const arg = *i; ++i) {
      if (arg[0] != '-') {
        break;
      } else if (strEq(arg, "--tokenize")) {
        mode = Mode::TOKENIZE;
      } else if (strEq(arg, "--parse")) {
        mode = Mode::PARSE;
      } else if (strEq(arg, "--print-ast")) {
        mode = Mode::PRINT_AST;
      } else if (strEq(arg, "--compile")) {
        mode = Mode::COMPILE;
      } else if (strEq(arg, "--optimize")) {
        mode = Mode::OPTIMIZE;
      } else if (strEq(arg, "-")) {
        break;
      } else if (strEq(arg, "--")) {
        ++i;
        break;
      } else {
        errorf("unknown argument '%s'", arg);
      }
    }

    if (!*i)
      errorf("no input files specified");

    if (!hasNewErrors()) {
      for (; char const *name = *i; ++i) {
        try {
          c4::input in{name};
	  c4::lexer l{in.begin(), in.end(), in.name()};
          switch (mode) {
          case Mode::TOKENIZE: { 
	    tokenize(l);  
	    break;
          }
          case Mode::PARSE: {
	    parse(l);
            break;
          }
          case Mode::PRINT_AST: {
	    print_ast(l);
            break;
          }
	  case Mode::COMPILE: {
            auto m = build_module(l, name);
            if(m)
              write_module(*m, name);
	    break;
	  }
	  case Mode::OPTIMIZE: {
            auto m = build_module(l, name);
            if(m) {
              write_module(*m, name);
              optimize(*m);
              // append _opt to the name
              std::string opt_name = name;
              if(opt_name == "-") {
                write_module(*m, opt_name.c_str());
              } else {
                auto pos = opt_name.find('.');
                if(pos == std::string::npos) {
                  opt_name.append("_opt"); // no . in filename just append the extension
                } else {
                  opt_name.replace(pos, std::string::npos, "_opt");
                }
                write_module(*m, opt_name.c_str());
              }
            }
	    break;
	  }
          }
        } catch(c4::input_error& ex) {
          errorErrno(c4::Pos{name});
        }
      }
    }
  } catch (std::exception const& e) {
    errorf("caught exception: %s", e.what());
  } catch (...) {
    errorf("caught unknown exception");
  }

  return printDiagnosticSummary();
}


void tokenize(c4::lexer& l) {
  auto token = l.get_token();
  std::cout.sync_with_stdio(false);
  while (token.first.type != c4::token_type::T_EOF) {
    if (token.first.type != c4::token_type::COMMENT 
	&& token.first.type != c4::token_type::INVALID
	&& token.first.errors.empty()) {
      std::cout << token.second << ' ' << token.first << '\n';
    }
    token = l.advance_token();
  }
  std::cout.sync_with_stdio(true);
  std::cout.flush();
}

std::unique_ptr<c4::ast_node> parse(c4::lexer& l) {
  c4::parser p{&l};
  std::unique_ptr<c4::ast_node> tu{p.get_ast_node()};
  if(!hasErrors()) {
    c4::sema_visitor sv;
    sv.visit(tu.get());
  }
  return std::move(tu);
}

void print_ast(c4::lexer& l) {
  auto tu = parse(l);   
  // Only print the AST when we are syntactically error-free.
  if(!hasErrors()) {
    c4::print_visitor pv{std::cout};
    pv.visit(tu.get());
  }
}

std::unique_ptr<llvm::Module> build_module(c4::lexer& l, const char* name) {
  using namespace llvm;
  auto tu = parse(l); // TODO this shouldn't do the parsing itself, composing stuff is cooler
  if(hasErrors()) return nullptr;

  LLVMContext &ctx = getGlobalContext();
  auto m = make_unique<Module>(name, ctx);
  if(!c4::compile(tu.get(), *m, ctx)) {
    errorf("Error during code generation!");
    return nullptr;
  }

  return m;
}


void write_module(const llvm::Module& m, const char* name) {
  if(strcmp(name, "-") == 0) {
    m.dump();
    return;
  }

  std::string errorStr;
  // strip the suffix and replace it
  std::string module_name{name};
  auto pos = module_name.find('.');
  if(pos == std::string::npos) {
    module_name.append(".ll"); // no . in filename just append the extension
  } else {
    module_name.replace(pos, std::string::npos, ".ll");
  }

  llvm::raw_fd_ostream stream{module_name.c_str(), errorStr};
  m.print(stream, nullptr);
  if(!errorStr.empty()) {
    errorf(errorStr.c_str());
  }
}
  
void optimize(llvm::Module& m) {
  c4::optimize(m);
}
