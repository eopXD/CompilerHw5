#ifndef __CODE_GEN_H__ 
#define __CODE_GEN_H__ 

#define REGISTER_NUM 32


// mama node
void codegen ( AST_NODE *program );

// reg related
void gen_prologue(char* name); // for function save register
void gen_epilogue(char* name); // for function end save register 
void get_reg();  // get the free register
void free_reg(); // free the unused reg

// help me sort this !?
void gen_head(char *name); // for declare the function name

// offset analysis
void get_offset(); // get the offset of the local variable
void param_offset ( AST_NODE *param_node, int offset ); // parameter
void block_offset ( AST_NODE *blockNode, int offset ); // block


// more general node
void gen_general(AST_NODE* node);
void gen_block(AST_NODE* blockNode);

// stmt node
void gen_stmt(AST_NODE* stmtNode); // switch the stmt node
void gen_assignStmt(AST_NODE* assignNode);
void gen_ifStmt(AST_NODE* ifNode);
void gen_whileStmt(AST_NODE* whileNode);
void gen_returnStmt(AST_NODE* returnNode);

// expr node
void gen_expr(AST_NODE* exprNode);

void gen_write(AST_NODE* writeNode);
void gen_read(AST_NODE* readNode);

// decl node
void gen_Decl(AST_NODE* DeclNode); // switch the decl node
void gen_varDecl(AST_NODE* varDeclDimList);
void gen_global_varDecl ( AST_NODE* varDeclDimList );

// func related
void gen_func(AST_NODE* func); // function call
void gen_funcDecl(AST_NODE* funcDeclNode); // function declare

#endif
