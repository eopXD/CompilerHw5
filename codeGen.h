#ifndef __CODE_GEN_H__ 
#define __CODE_GEN_H__ 

#define REGISTER_NUM 64

#include "header.h"
#include "symbolTable.h"

// iteration on all childs of 'node' with pointer 'childnode'
#define FOR_ALL_CHILD(node, childnode) for(AST_NODE* (childnode)=(node)->child; (childnode)!=NULL; (childnode)=(childnode)->rightSibling)

/* kind of node */
#define id_kind(idNode) (idNode)->semantic_value.identifierSemanticValue.kind
#define stmt_kind(stmtNode) (stmtNode)->semantic_value.stmtSemanticValue.kind
#define expr_kind(exprNode) (exprNode)->semantic_value.exprSemanticValue.kind
#define decl_kind(declNode) (declNode)->semantic_value.declSemanticValue.kind

/* identifier related */
#define id_entry(idNode) (idNode)->semantic_value.identifierSemanticValue.symbolTableEntry
#define id_nest_level(idNode) (idNode)->semantic_value.identifierSemanticValue.symbolTableEntry->nestingLevel
#define id_name(idNode) (idNode)->semantic_value.identifierSemanticValue.identifierName
#define id_offset(idNode) (idNode)->semantic_value.identifierSemanticValue.symbolTableEntry->offset

/* expression operation */
#define bin_op(node) (node)->semantic_value.exprSemanticValue.op.binaryOp
#define un_op(node) (node)->semantic_value.exprSemanticValue.op.unaryOp

/* constant */
#define const_intval(node) (node)->semantic_value.const1->const_u.intval
#define const_fval(node) (node)->semantic_value.const1->const_u.fval
#define const_type(node) (node)->semantic_value.const1->const_type

// for the reg tracking 
typedef enum regAttributeKind
{
   TEMPORARY_KIND,
   VARIABLE_KIND, 
   CONST_KIND,
   ADDRESS_KIND,
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

// more general node
void gen_block(AST_NODE* blockNode);

// stmt node
void gen_stmt(AST_NODE* stmtNode); // switch the stmt node
void gen_assignStmt(AST_NODE* assignNode);
void gen_ifStmt(AST_NODE* ifNode);
void gen_whileStmt(AST_NODE* whileNode);
void gen_returnStmt(AST_NODE* returnNode);

// expr node
int gen_expr(AST_NODE* exprNode); // return the reg of the expr

// decl node
void gen_global_varDecl ( AST_NODE* varDeclDimList );

// func related
void gen_prologue ( char *func_name );
void gen_epilogue (AST_NODE* node ,char *func_name );
void callee_save ();
void callee_restore ();
void gen_func(AST_NODE* funcNode); // function call
void gen_funcDecl(AST_NODE* funcDeclNode); // function declare

#endif
