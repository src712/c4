BUILDDIR ?= build
CFG      ?= default
LLVM_CONFIG ?= llvm-config
NAME     ?= c4
SRCDIR   ?= src
TESTDIR   ?= test

-include $(CFG).cfg

Q ?= @

BINDIR := $(BUILDDIR)/$(CFG)
BIN    := $(BINDIR)/$(NAME)
SRC    := $(sort $(shell find $(SRCDIR) -name '*.cc'))
OBJ    := $(SRC:$(SRCDIR)/%.cc=$(BINDIR)/%.o)
DEP    := $(OBJ:%.o=%.d)

LEXERTESTS := $(sort $(wildcard $(TESTDIR)/lexer/*.test))
LEXERRESULTS := $(sort $(wildcard $(TESTDIR)/lexer/*.exp))
PARSERTESTS := $(sort $(wildcard $(TESTDIR)/parser/*.test))
PARSERRESULTS := $(sort $(wildcard $(TESTDIR)/parser/*.exp))
PRINTERTESTS := $(sort $(wildcard $(TESTDIR)/print_ast/*.test))
PRINTERRESULTS := $(sort $(wildcard $(TESTDIR)/print_ast/*.exp))
COMPILERTESTS := $(sort $(wildcard $(TESTDIR)/compiler/*.test))
COMPILERRESULTS := $(sort $(wildcard $(TESTDIR)/compiler/*.exp))
LEXARG 	   := --tokenize
PARSEARG   := --parse
PRINTARG   := --print-ast
COMPILEARG   := --compile

LLVM_CFLAGS  := $(shell $(LLVM_CONFIG) --cppflags)
LLVM_LDFLAGS := $(shell $(LLVM_CONFIG) --libs core transformutils) $(shell $(LLVM_CONFIG) --ldflags)
LLVM_BINDIR  := $(shell $(LLVM_CONFIG) --bindir)

CFLAGS   += $(LLVM_CFLAGS) -Wall -Wextra -Werror -DNDEBUG -O3
CXXFLAGS += $(CFLAGS) -std=c++11
LDFLAGS  += $(LLVM_LDFLAGS)

DUMMY := $(shell mkdir -p $(sort $(dir $(OBJ))))

.PHONY: all clean

all: $(BIN)

-include $(DEP)

FORCE:

clean:
	@echo "===> CLEAN"
	$(Q)rm -fr $(BINDIR)

test/lexer/%.test: test/lexer/%.exp $(BIN) FORCE
	@echo "===> Testing $@"
	$(shell $(BINDIR)/$(NAME) $(LEXARG) $@ > $(TESTDIR)/result.tmp 2>&1)
	@if diff $(TESTDIR)/result.tmp $< >/dev/null; then \
	echo "PASSED"; \
	expr "`cat $(TESTDIR)/success.tmp`" + 1 > $(TESTDIR)/success.tmp; \
	else \
	echo "FAILED"; \
	expr "`cat $(TESTDIR)/failure.tmp`" + 1 > $(TESTDIR)/failure.tmp; \
	echo "`diff $(TESTDIR)/result.tmp $<;`"; \
	fi
	@rm $(TESTDIR)/result.tmp

test/parser/%.test: test/parser/%.exp $(BIN) FORCE
	@echo "===> Testing $@"
	$(shell $(BINDIR)/$(NAME) $(PARSEARG) $@ > $(TESTDIR)/result.tmp 2>&1)
	@if diff $(TESTDIR)/result.tmp $< >/dev/null; then \
	echo "PASSED"; \
	expr "`cat $(TESTDIR)/success.tmp`" + 1 > $(TESTDIR)/success.tmp; \
	else \
	echo "FAILED"; \
	expr "`cat $(TESTDIR)/failure.tmp`" + 1 > $(TESTDIR)/failure.tmp; \
	echo "`diff $(TESTDIR)/result.tmp $<;`"; \
	fi
	@rm $(TESTDIR)/result.tmp

test/print_ast/%.test: test/print_ast/%.exp $(BIN) FORCE
	@echo "===> Testing $@"
	$(shell $(BINDIR)/$(NAME) $(PRINTARG) $@ > $(TESTDIR)/result.tmp 2>&1)
	@if diff $(TESTDIR)/result.tmp $< >/dev/null; then \
	echo "PASSED"; \
	expr "`cat $(TESTDIR)/success.tmp`" + 1 > $(TESTDIR)/success.tmp; \
	else \
	echo "FAILED"; \
	expr "`cat $(TESTDIR)/failure.tmp`" + 1 > $(TESTDIR)/failure.tmp; \
	echo "`diff $(TESTDIR)/result.tmp $<;`"; \
	fi
	@rm $(TESTDIR)/result.tmp

test/compiler/%.test: test/compiler/%.exp $(BIN) FORCE
	@echo "===> Testing $@"
	$(eval LLIFILE = $(subst .test,.ll,$@))
	@echo "===> Using $(LLVM_BINDIR)/lli $(LLIFILE)"
	$(shell $(BINDIR)/$(NAME) $(COMPILEARG) $@ && $(LLVM_BINDIR)/lli $(LLIFILE) || echo "$$?" > $(TESTDIR)/result.tmp)
	@if diff $(TESTDIR)/result.tmp $< >/dev/null; then \
	echo "PASSED"; \
	expr "`cat $(TESTDIR)/success.tmp`" + 1 > $(TESTDIR)/success.tmp; \
	else \
	echo "FAILED"; \
	expr "`cat $(TESTDIR)/failure.tmp`" + 1 > $(TESTDIR)/failure.tmp; \
	echo "`diff $(TESTDIR)/result.tmp $<;`"; \
	fi
	@rm $(LLIFILE)
	@rm $(TESTDIR)/result.tmp

pre_compilertest:
	@echo "===> Testing Compiler"
	@echo "0" > $(TESTDIR)/success.tmp
	@echo "0" > $(TESTDIR)/failure.tmp

test_compiler: pre_compilertest $(COMPILERTESTS)
	@echo "===> Compiler Test Summary"
	@echo "Succeeded tests: `cat $(TESTDIR)/success.tmp`"
	@echo "Failed tests: `cat $(TESTDIR)/failure.tmp`"
	@rm $(TESTDIR)/success.tmp
	@rm $(TESTDIR)/failure.tmp

pre_printertest:
	@echo "===> Testing Pretty Printer"
	@echo "0" > $(TESTDIR)/success.tmp
	@echo "0" > $(TESTDIR)/failure.tmp

test_printer: pre_printertest $(PRINTERTESTS)
	@echo "===> Pretty Printer Test Summary"
	@echo "Succeeded tests: `cat $(TESTDIR)/success.tmp`"
	@echo "Failed tests: `cat $(TESTDIR)/failure.tmp`"
	@rm $(TESTDIR)/success.tmp
	@rm $(TESTDIR)/failure.tmp

pre_parsertest:
	@echo "===> Testing Parser"
	@echo "0" > $(TESTDIR)/success.tmp
	@echo "0" > $(TESTDIR)/failure.tmp

test_parser: pre_parsertest $(PARSERTESTS)
	@echo "===> Parser Test Summary"
	@echo "Succeeded tests: `cat $(TESTDIR)/success.tmp`"
	@echo "Failed tests: `cat $(TESTDIR)/failure.tmp`"
	@rm $(TESTDIR)/success.tmp
	@rm $(TESTDIR)/failure.tmp

pre_lexertest:
	@echo "===> Testing Lexer"
	@echo "0" > $(TESTDIR)/success.tmp
	@echo "0" > $(TESTDIR)/failure.tmp

test_lexer: pre_lexertest $(LEXERTESTS)
	@echo "===> Lexer Test Summary"
	@echo "Succeeded tests: `cat $(TESTDIR)/success.tmp`"
	@echo "Failed tests: `cat $(TESTDIR)/failure.tmp`"
	@rm $(TESTDIR)/success.tmp
	@rm $(TESTDIR)/failure.tmp

$(BIN): $(OBJ)
	@echo "===> LD $@"
	$(Q)$(CXX) -o $(BIN) $(OBJ) $(LDFLAGS)

$(BINDIR)/%.o: $(SRCDIR)/%.cc
	@echo "===> CXX $<"
	$(Q)$(CXX) $(CXXFLAGS) -MMD -c -o $@ $<

presentation: presentation.tex
	@echo "===> Running pdflatex $<"
	pdflatex -interaction nonstopmode -file-line-error -output-directory=/tmp presentation.tex
