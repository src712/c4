#ifndef C4_AST_FWD_H
#define C4_AST_FWD_H

namespace c4 {

struct ast_node;

struct translation_unit;
struct declarator;
struct base_decl;
struct parameter_decl;
struct decl;
struct type_specifier;
struct type_name;
struct struct_specifier;

struct stmt;
struct comp_stmt;
struct break_stmt;
struct continue_stmt;
struct return_stmt;
struct labeled_stmt;
struct goto_stmt;
struct while_stmt;
struct if_stmt;
struct if_else_stmt;
struct expr_stmt;

struct expression;
struct primary_expression;
struct sizeof_expr;
struct sizeof_type;
struct unary_operator;
struct binary_operator;
struct postfix_operator;
struct subscript_operator;
struct function_call;
struct ternary_expr;

} // c4
#endif /* C4_AST_FWD_H */
