#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "header.h"
#include "symbolTable.h"
#include "codeGen.h"

#define FOR_ALL_CHILD(node, childnode) for(AST_NODE* childnode=node->child; childnode!=NULL; childnode=childnode->rightSibling)

int label_no = 0; // for nested structure
FILE* write_file;
int constant_value_counter = 0; // for constant allocation
char* current_function;

SymbolTableEntry* get_entry(AST_NODE* node)
{
    return node->semantic_value.identifierSemanticValue.symbolTableEntry;
}


void gen_stmt(AST_NODE* stmtNode)
{
    if(stmtNode->nodeType == NUL_NODE){
        return ;
    }else if(stmtNode->nodeType == BLOCK_NODE){
        gen_block(stmtNode);
    }
    else{
        switch(stmtNode->semantic_value.stmtSemanticValue.kind){
            case WHILE_STMT:
                gen_whileStmt(stmtNode);
                break;
            case FOR_STMT:
                printf("QQQQ\n NO FOR\n");
                break;
            case ASSIGN_STMT:
                gen_assignStmt(stmtNode);
                break;
            case RETURN_STMT:
                gen_returnStmt(stmtNode); 
                break;
            case FUNCTION_CALL_STMT:
                gen_func(stmtNode);
                break;
            case IF_STMT:
                gen_ifStmt(stmtNode);
                break;
            default:
                printf("QQQQQ\n ERROR\n");
                break;
        }
    }
}

void gen_assignStmt(AST_NODE* assignNode)
{
    AST_NODE* leftOp = assignNode->child;
    AST_NODE* rightOp = leftOp->rightSibling;

    gen_expr(rightOp);
    SymbolTableEntry* entry = get_entry(rightOp);
    SymbolTableEntry* entry2 = get_entry(leftOp);
    if(entry2->place == -1){
        get_reg(leftOp);
    }
    fprintf(write_file, "mv t%d, t%d\n", entry2->place, entry->place);
    // TODO live tracking 
}

void gen_returnStmt(AST_NODE* returnNode)
{
    gen_expr(returnNode->child);
    SymbolTableEntry* entry = get_entry(returnNode->child);
    fprintf(write_file, "mv a0, t%d\n", entry->place);
    fprintf(write_file, "j _end_%s\n", current_function);
}

void gen_ifStmt(AST_NODE* ifNode)
{
    int local_label_number = label_no++;
    AST_NODE* boolExpression = ifNode->child;
    gen_expr(boolExpression); 
    SymbolTableEntry* entry = get_entry(boolExpression);
    AST_NODE* ifBodyNode = ifNode->child->rightSibling;
    AST_NODE* elseBodyNode = ifBodyNode->rightSibling;
    if(elseBodyNode == NULL){
        fprintf(write_file, "beqz X%d, _Lexit%d\n", entry->place, local_label_number);
        gen_stmt(ifBodyNode);
        fprintf(write_file, "_Lexit%d\n", local_label_number);

    }else{
        fprintf(write_file, "beqz X%d, _Lelse%d  \n", entry->place, local_label_number);
        gen_stmt(ifBodyNode);
        fprintf(write_file, "j _Lexit%d\n", local_label_number);
        fprintf(write_file, "_Lelse%d:\n", local_label_number);
        gen_stmt(elseBodyNode);
        fprintf(write_file, " _Lexit%d\n", local_label_number);
    }
}

void gen_whileStmt(AST_NODE* whileNode)
{
    int local_label_number = label_no++;
    fprintf(write_file, "_Test%d: ", local_label_number);
    AST_NODE* boolExpression = whileNode->child; 
    gen_expr(boolExpression);
    SymbolTableEntry* entry = get_entry(boolExpression);
    fprintf(write_file, "beqz X%d, _Lexit%d\n", entry->place, local_label_number); 
    gen_stmt(boolExpression->rightSibling);
    fprintf(write_file, "j _Test%d\n", local_label_number);
    fprintf(write_file, "_Lexit%d\n", local_label_number);
}

void gen_expr(AST_NODE* exprNode)
{

}

void gen_block ( AST_NODE *blockNode ) 
{
      int frameSize = 0;
      FOR_ALL_CHILD(blockNode, child) {
          if ( child->nodeType == VARIABLE_DECL_LIST_NODE ) {
              FOR_ALL_CHILD(child, declNode) {
                  if ( declNode->semantic_value.declSemanticValue.kind == VARIABLE_DECL ) {
                      gen_varDecl(declNode);
                  }
              }
          } else if ( child->nodeType == STMT_LIST_NODE ) {
              AST_NODE *stmtNode = child->child;
              FOR_ALL_CHILD(child, stmtNode) {
                  gen_stmt(stmtNode);
              }
          }
      }
}

