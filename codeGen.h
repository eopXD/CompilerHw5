#ifndef __CODE_GEN_H__ 
#define __CODE_GEN_H__ 

#define REGISTER_NUM 64

#include "header.h"
#include "symbolTable.h"

// for the reg tracking 
typedef enum regAttributeKind
{
   TEMPORARY_KIND,
   VARIABLE_KIND, 
   CONST_KIND,
   OTHER_KIND
} regAttributeKind;

typedef struct RegTable{
    regAttributeKind kind;
    AST_NODE* node;
    int status;
} RegTable;

// print binary representation of float
union ufloat {
	float f;
	unsigned u;
};

// mama node
void codegen ( AST_NODE *program );

// some useful function 
SymbolTableEntry* get_entry(AST_NODE* node);

// reg related
int get_reg( AST_NODE* node);  // get the free register
int in_reg ( AST_NODE* node); // check if node already inside register
int get_int_reg (AST_NODE* node);
int get_float_reg (AST_NODE* node);

void free_reg(int reg); // free the unused reg

// offset analysis
void offsetgen ( AST_NODE *program ); // mama call
int gen_offset ( AST_NODE *node, int offset ); // general node
int block_offset ( AST_NODE *blockNode, int offset ); // block
void param_offset ( AST_NODE *paramNode, int offset ); // parameter

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
void gen_constValue(AST_NODE* constValue);
void gen_variableRValue(AST_NODE* variable);
int gen_array_addr ( AST_NODE *exprNode );
int gen_expr(AST_NODE* exprNode); // return the reg of the expr

void gen_write(AST_NODE* writeNode);
void gen_read(AST_NODE* readNode);

// decl node
void gen_decl(AST_NODE* DeclNode); // switch the decl node
void gen_varDecl(AST_NODE* varDeclDimList);
void gen_global_varDecl ( AST_NODE* varDeclDimList );

// func related
void gen_prologue ( char *func_name );
void gen_epilogue (AST_NODE* node ,char *func_name );
void callee_save ();
void callee_restore ();
void gen_func(AST_NODE* funcNode); // function call
void gen_funcDecl(AST_NODE* funcDeclNode); // function declare

#endif
