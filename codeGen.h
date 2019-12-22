#ifndef __CODE_GEN_H__ 
#define __CODE_GEN_H__ 

#define REGISTER_NUM 32

void gen_prologue(char* name); // for function save register
void gen_head(char *name); // for declare the function name
void gen_epilogue(char* name); // for function end save register 
void get_reg();  // get the free register
void get_offset(); // get the offset of the local variable
void free_reg(); // free the unused reg
void gen_block(AST_NODE* blockNode);
void gen_general(AST_NODE* node);
void gen_func(AST_NODE* func);
void gen_expr(AST_NODE* exprNode);
void gen_assignStmt(AST_NODE* assingNode);
void gen_stmt(AST_NODE* stmtNode); // switch the stmt node
void gen_ifStmt(AST_NODE* ifNode);
void gen_whileStmt(AST_NODE* whileNode);
void gen_write(AST_NODE* writeNode);
void gen_read(AST_NODE* readNode);
void gen_varDecl(AST_NODE* varDeclDimList);
void gen_funcDecl(AST_NODE* funcDeclNode);
void gen_returnStmt(AST_NODE* returnNode);
void gen_Decl(AST_NODE* DeclNode); // switch the decl node

#endif
