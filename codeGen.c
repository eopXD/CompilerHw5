#include "header.h"
#include "codeGen.h"
#include "symbolTable.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DIRTY 1
#define CLEAN 0
#define FREE -1
#define BUSY -2
#define REG_FT0 32
#define REG_T0 5
#define REG_S1 9 
#define FLOAT_REG_BEGIN 32
int label_no = 0;
FILE *write_file;
int constant_value_counter = 0; // for constant allocation
char *current_function; // current function name
RegTable regTable[REGISTER_NUM];

char regName[64][8] = {
  "zero", "ra", "sp", "gp", "tp",
  "t0", "t1", "t2", "fp", "s1",
  "a0", "a1", "a2", "a3", "a4",
  "a5", "a6", "a7", "s2", "s3",
  "s4", "s5", "s6", "s7", "s8",
  "s9", "s10", "s11", "t3", "t4",
  "t5", "t6", "ft0", "ft1", "ft2",
  "ft3", "ft4", "ft5", "ft6", "ft7",
  "fs0", "fs1", "fa0", "fa1", "fa2",
  "fa3", "fa4", "fa5", "fa6", "fa7",
  "fs2", "fs3", "fs4", "fs5", "fs6",
  "fs7", "fs8", "fs9", "fs10", "fs11",
  "ft8", "ft9", "ft10", "ft11"
};
int caller_save_list[64]={
  0, 1, 0, 0, 0,
  1, 1, 1, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0,
  0, 0, 0, 1, 1,
  1, 1, 1, 1, 1,
  0, 0, 1, 1, 1,
  1, 1, 1, 1, 1,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0
};
int useRegList[64] = { 
  1, 1, 1, 1, 1,
  0, 0, 0, 1, 1,
  1, 1, 1, 1, 1,  
  1, 1, 1, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  1, 1, 1, 1, 1,
  1, 1, 1, 1, 1,
  1, 1, 1, 1, 1,
  1, 1, 1, 1, 1,
  1, 1, 1, 1
};

void copy_reg(RegTable *a, RegTable *b){
  for(int i = 0; i < REGISTER_NUM; i ++){
    a[i] = b[i];
  }
  return ;
}

int in_reg(AST_NODE* node)
{
  if(node->nodeType == IDENTIFIER_NODE && node->semantic_value.identifierSemanticValue.kind == NORMAL_ID ){
    SymbolTableEntry *entry = id_entry(node);
    char *name = id_name(node);
    if(entry->nestingLevel == 0){
      return -1;
    }
    for(int i = 0; i < REGISTER_NUM; i++){
      if(regTable[i].kind == VARIABLE_KIND && regTable[i].status != FREE){
        SymbolTableEntry *tmp = id_entry(regTable[i].node);
        if(!strcmp(entry->name, tmp->name)){
          return i;
        }
      }
    }
    return -1;
  }else if(node->nodeType == CONST_VALUE_NODE){
    if ( node->semantic_value.const1->const_type == INTEGERC ) { // const float
      for(int i = 0; i < REGISTER_NUM; i++){
        if(regTable[i].kind == CONST_KIND && regTable[i].node == node){
          return i;
        }
      }
    }
  }
  return -1;
}

void store_reg(AST_NODE* node, int index)
{
  if(node == NULL){
    regTable[index].kind = ADDRESS_KIND;
    regTable[index].status = CLEAN;
    return ;
  }
  regTable[index].node = node;
  if(node->nodeType == IDENTIFIER_NODE){
    regTable[index].kind = VARIABLE_KIND;
    regTable[index].status = CLEAN;
  }else if(node->nodeType == CONST_VALUE_NODE){
    regTable[index].kind = CONST_KIND;
    regTable[index].status = CLEAN;
  }else{
    regTable[index].kind = TEMPORARY_KIND;
    regTable[index].status = DIRTY;
  }
}

void initial_reg()
{
  for(int i = 0; i < REGISTER_NUM; i++){
    if(useRegList[i]){
      regTable[i].status = DIRTY;
      regTable[i].kind = OTHER_KIND;
    }else{
      regTable[i].status = FREE;
    }
  }
}
int get_float_reg(AST_NODE* node)
{
  int index = in_reg(node);
  if(index > 0){
    return index;
  }
  
  for(int i = FLOAT_REG_BEGIN; i < REGISTER_NUM; i++){
    if(regTable[i].status == FREE){ // get the free register
      store_reg(node, i);
      return i;
    }
  }
  for(int i = FLOAT_REG_BEGIN; i < REGISTER_NUM; i++){
    if(regTable[i].status == CLEAN){// get the clean register
      free_reg(i);
      store_reg(node, i);
      return i;
    }
  }
  for(int i = FLOAT_REG_BEGIN; i < REGISTER_NUM; i++){ // get dirty register and only variable could be store don't kill the temporaries
    if(regTable[i].status == DIRTY && regTable[i].kind != TEMPORARY_KIND && regTable[i].kind != OTHER_KIND){
      free_reg(i);
      store_reg(node, i);
      return i;
    }
  }
  return -1;
}

int get_int_reg(AST_NODE* node)
{
  int index = in_reg(node);
  if(index > 0){
    return index;
  }

  for(int i = 0; i < FLOAT_REG_BEGIN; i++){
    if(regTable[i].status == FREE){
      store_reg(node, i);
      return i;
    }
  }
  for(int i = 0; i < FLOAT_REG_BEGIN; i++){
    if(regTable[i].status == CLEAN){
      free_reg(i);
      store_reg(node, i);
      return i;
    }
  }
  for(int i = 0; i < FLOAT_REG_BEGIN; i++){
    if(regTable[i].status == DIRTY && regTable[i].kind != TEMPORARY_KIND && regTable[i].kind != OTHER_KIND){
      free_reg(i);
      store_reg(node, i);
      return i;
    }
  }
  return -1;
}

void free_reg ( int regIndex ) {
  char *float_or_not; 
  RegTable reg = regTable[regIndex];
  int addr_reg;
  if ( reg.status == DIRTY ) {
    if ( reg.kind == VARIABLE_KIND && reg.node->nodeType == IDENTIFIER_NODE ) {  //only variale could be store
      regTable[regIndex].status = BUSY; // protect the reg to be get again
      AST_NODE *node = reg.node;
      SymbolTableEntry *entry = id_entry(reg.node);
      float_or_not = reg.node->dataType == INT_TYPE ? "" : "f";
      addr_reg = REG_S1; 
      fprintf(stderr, "[free_reg] writing data for var %s\n", id_name(node));
      fprintf(stderr, "[free_reg] (regIndex, addr_reg): (%d %d)\n", regIndex, addr_reg);
      if ( id_kind(node) == NORMAL_ID ) { // normal var
        if ( entry->nestingLevel == 0 ) { // global normal
          fprintf(write_file, "la %s, _g_%s\n", regName[addr_reg], id_name(node));
          fprintf(write_file, "%ssw %s, 0(%s)\n", float_or_not, regName[regIndex], regName[addr_reg]);
        } else { // local normal
          fprintf(write_file, "%ssw %s, -%d(fp)\n", float_or_not, regName[regIndex], id_offset(node));
        }
      } else if ( id_kind(node) == ARRAY_ID ) { // array var 
      // solve array offset with gen_array_addr()
        if ( id_nest_level(node) == 0 ) { // global array
          fprintf(write_file, "lui %s, %%hi(_g_%s)\n", regName[addr_reg], id_name(node));
          fprintf(write_file, "addi %s, %s, %%lo(_g_%s)\n", regName[addr_reg], regName[addr_reg], id_name(node));
          fprintf(write_file, "%ssw %s, %d(%s)\n", float_or_not, regName[regIndex], 4*const_intval(node->child), regName[addr_reg]);
        } else { // local array
            AST_NODE *dimListNode = reg.node->child;
            SymbolTableEntry *entry = id_entry(reg.node);
            fprintf(write_file, "%ssw %s, -%d(fp)\n", float_or_not, regName[regIndex], 4*const_intval(dimListNode)+entry->offset);
        }
        } else {
            fprintf(stderr, "[free_reg] receive bad node SemanticValueKind\n");
            exit(1);
        }
    }
  }
  regTable[regIndex].node = NULL;
  regTable[regIndex].status = FREE;
}

void gen_stmt(AST_NODE* stmtNode)
{
  if(stmtNode->nodeType == NUL_NODE){
    return ;
  }else if(stmtNode->nodeType == BLOCK_NODE){
    gen_block(stmtNode);
  }
  else{
    if(stmt_kind(stmtNode) == FUNCTION_CALL_STMT){
      gen_func(stmtNode);
    }else{
      switch(stmt_kind(stmtNode)){
        case WHILE_STMT:
          gen_whileStmt(stmtNode);
          break;
        case FOR_STMT:
          printf("[gen_stmt] FOR_STMT is not yet implemented\n");
          break;
        case ASSIGN_STMT:
          gen_assignStmt(stmtNode);
          break;
        case RETURN_STMT:
          gen_returnStmt(stmtNode);
          break;
        case FUNCTION_CALL_STMT:
            break;
        case IF_STMT:
          gen_ifStmt(stmtNode);
          break;
        default:
          printf("[gen_stmt] recieve unknown stmtNode ERROR\n");
          break;
      }
    }
  }
}

void gen_assignStmt(AST_NODE* assignNode)
{   
  AST_NODE *lhs = assignNode->child;
  int rhs_reg = gen_expr(lhs->rightSibling); // acquire rhs as register
  store_reg(lhs, rhs_reg);

  regTable[rhs_reg].status = DIRTY;
  if(lhs->nodeType == IDENTIFIER_NODE){
      SymbolTableEntry *entry = id_entry(lhs);
      if(entry->nestingLevel == 0 || id_kind(lhs) == ARRAY_ID){
          free_reg(rhs_reg);
      }
  }
}

void gen_returnStmt(AST_NODE* returnNode)
{
      int index = gen_expr(returnNode->child);
      SymbolTableEntry *entry = id_entry(returnNode->child);
      fprintf(write_file, "mv a0, %s\n", regName[index]);
      fprintf(write_file, "j _end_%s\n", current_function);
}

void gen_ifStmt(AST_NODE* ifNode)
{
      int local_label_number = label_no++;
      AST_NODE* boolExpression = ifNode->child;
      int index = gen_expr(boolExpression);
      AST_NODE* ifBodyNode = ifNode->child->rightSibling;
      AST_NODE* elseBodyNode = ifBodyNode->rightSibling;
      if(elseBodyNode == NULL){
          fprintf(write_file, "beqz %s, _Lexit%d\n", regName[index], local_label_number);
          gen_block(ifBodyNode);
          fprintf(write_file, "_Lexit%d:\n", local_label_number);
  
      }else{
          fprintf(write_file, "beqz %s, _Lelse%d  \n", regName[index], local_label_number);
          gen_block(ifBodyNode);
          fprintf(write_file, "j _Lexit%d\n", local_label_number);
          fprintf(write_file, "_Lelse%d:\n", local_label_number);
          gen_block(elseBodyNode);
          fprintf(write_file, " _Lexit%d:\n", local_label_number);
      }
}

void gen_whileStmt(AST_NODE* whileNode)
{
    int local_label_number = label_no++;
    fprintf(write_file, "_Test%d: ", local_label_number);
    AST_NODE* boolExpression = whileNode->child;
    int index = gen_expr(boolExpression);
    fprintf(write_file, "beqz %s, _Lexit%d\n", regName[index], local_label_number);
    gen_block(boolExpression->rightSibling);
    fprintf(write_file, "j _Test%d\n", local_label_number);
    fprintf(write_file, "_Lexit%d:\n", local_label_number);
}
void codegen ( AST_NODE *program ) {
  write_file = fopen("output.s", "w");
  if ( write_file == NULL ) {
    fprintf(stderr, "[codegen] open write file fail\n");
    exit(1);
  }
  constant_value_counter = 1;

  FOR_ALL_CHILD(program, child) {
    if ( child->nodeType == VARIABLE_DECL_LIST_NODE ) { // global decl
      gen_global_varDecl(child);
    } else if ( child->nodeType == DECLARATION_NODE ) { // global function (including main)
      RegTable tmp[REGISTER_NUM];
      copy_reg(tmp, regTable);
      gen_funcDecl(child);
      copy_reg(regTable, tmp);
    }
  }
  fclose(write_file);
}

// expression returns register for use
int gen_expr ( AST_NODE *exprNode ) {
  fprintf(stderr, "[gen_expr] start\n");

  // results put into 'rs'
  int rs, rt, rd;
  char *float_or_not;
  char *float_or_not2;

  int ival;

  union ufloat uf;
  char str[256] = {};

  if ( exprNode->nodeType == CONST_VALUE_NODE ) { // constant value
    fprintf(stderr, "[gen_expr] CONST_VALUE_NODE\n");
    if ( const_type(exprNode) == INTEGERC ) { // const integer
      if ( (rs=in_reg(exprNode)) < 0 ) {
        rs = get_int_reg(exprNode);
        fprintf(write_file, ".data\n");
        fprintf(write_file, "_CONSTANT_%d: .word %d\n", constant_value_counter, const_intval(exprNode));
        fprintf(write_file, ".align 3\n");
        fprintf(write_file, ".text\n");
        fprintf(write_file, "lw %s, _CONSTANT_%d\n", regName[rs], constant_value_counter);
        ++constant_value_counter;
      }
    } else if ( const_type(exprNode) == FLOATC ) { // const float
      fprintf(stderr, "CONST_VALUE_NODE: FLOATC\n");
      if ( (rs=in_reg(exprNode)) < 0 ) {
        fprintf(stderr, "value is not in reg\n");
        uf.f = const_fval(exprNode);
        fprintf(stderr, "get reg for rs and rt\n");
        rs = get_float_reg(exprNode);
        rt = get_int_reg(exprNode);
        fprintf(stderr ," (rs, rt): %d %d\n", rs, rt);

        fprintf(write_file, ".data\n");
        fprintf(write_file, "_CONSTANT_%d:\n", constant_value_counter);
        fprintf(write_file, ".word %u\n", uf.u);
        fprintf(write_file, ".align 3\n");
        fprintf(write_file, ".text\n");
        fprintf(write_file, "lui %s, %%", regName[rt]);
        fprintf(write_file, "hi(_CONSTANT_%d)\n", constant_value_counter);
        fprintf(write_file, "flw %s, %%", regName[rs]);
        fprintf(write_file, "lo(_CONSTANT_%d)(%s)\n", constant_value_counter, regName[rt]);
        ++constant_value_counter;
        free_reg(rt);
        fprintf(stderr, "free reg for rt\n");

      }
    } else if ( const_type(exprNode) == STRINGC ) { // const string
      if ( (rs = in_reg(exprNode)) < 0 ) {
        rs = get_int_reg(exprNode);
        // because the string is "hello" need to cost the " in the front and button
        strncpy(str, exprNode->semantic_value.const1->const_u.sc+1, strlen(exprNode->semantic_value.const1->const_u.sc)-2);
        str[strlen(exprNode->semantic_value.const1->const_u.sc)-1] = '\0';
        fprintf(write_file, ".data\n");
        fprintf(write_file, "_CONSTANT_%d:\n", constant_value_counter);
        fprintf(write_file, ".ascii \"%s\\000\"\n", str);
        fprintf(write_file, ".align 3\n");
        fprintf(write_file, ".text\n");
        fprintf(write_file, "la %s, _CONSTANT_%d\n", regName[rs], constant_value_counter);
        ++constant_value_counter;
      }
    } else {
      fprintf(stderr, "[gen_expr] unknown const type\n");
      exit(1);
    }
  } else if ( exprNode->nodeType == EXPR_NODE ) { // solve expression
    fprintf(stderr, "[gen_expr] EXPR_NODE\n");
    // TODO: solve for constant folding
    if( expr_kind(exprNode) == BINARY_OPERATION ) {
      fprintf(stderr, "[gen_expr] EXPR_NODE - BINARY_OP\n");  
      float_or_not = exprNode->dataType == INT_TYPE ? "" : "f";
      float_or_not2 = exprNode->dataType == INT_TYPE ? "" : ".s";
      rs = gen_expr(exprNode->child);
      rt = gen_expr(exprNode->child->rightSibling);
      if ( bin_op(exprNode) == BINARY_OP_ADD ) {
        fprintf(write_file, "%sadd%s %s, %s, %s\n", float_or_not, float_or_not2, regName[rs], regName[rs], regName[rt]);
      } else if ( bin_op(exprNode) == BINARY_OP_SUB ) {
        fprintf(write_file, "%ssub%s %s, %s, %s\n", float_or_not, float_or_not2, regName[rs], regName[rs], regName[rt]);
      } else if ( bin_op(exprNode) == BINARY_OP_MUL ) {
        fprintf(write_file, "%smul%s %s, %s, %s\n", float_or_not, float_or_not2, regName[rs], regName[rs], regName[rt]);
      } else if ( bin_op(exprNode) == BINARY_OP_DIV ) {
        fprintf(write_file, "%sdiv%s %s, %s, %s\n", float_or_not, float_or_not2, regName[rs], regName[rs], regName[rt]);
      } else if ( bin_op(exprNode) == BINARY_OP_AND ) {
        if ( exprNode->dataType == FLOAT_TYPE ) {
          fprintf(stderr, "[gen_expr] exprNode BINARY_OP_AND is FLOAT_TYPE\n");
          exit(1);
        }
        fprintf(write_file, "and %s, %s, %s\n", regName[rs], regName[rs], regName[rt]);
      } else if ( bin_op(exprNode) == BINARY_OP_OR ) {
        if ( exprNode->dataType == FLOAT_TYPE ) {
          fprintf(stderr, "[gen_expr] exprNode BINARY_OP_OR is FLOAT_TYPE\n");
          exit(1);
        }
        fprintf(write_file, "or %s, %s, %s\n", regName[rs], regName[rs], regName[rt]);
      } else { // comparing operation 
        fprintf(stderr, "[gen_expr] EXPR_NODE - BINARY_OP - comparison op\n");  
        rd = get_int_reg(exprNode);
        char *comparison_op1, *comparison_op2;
        if ( bin_op(exprNode) == BINARY_OP_EQ ) {
          fprintf(stderr, "[gen_expr] EXPR_NODE - BINARY_OP_EQ\n");  
          comparison_op1 = (exprNode->dataType == INT_TYPE) ? "sub" : "feq.s";
          comparison_op2 = (exprNode->dataType == INT_TYPE) ? "seqz" : "snez";
          fprintf(write_file, "%s %s, %s, %s\n", comparison_op1, regName[rd], regName[rs], regName[rt]);
          fprintf(write_file, "%s %s, %s\n", comparison_op2, regName[rd], regName[rd]);
        } else if ( bin_op(exprNode) == BINARY_OP_NE ) {          
          fprintf(stderr, "[gen_expr] EXPR_NODE - BINARY_OP_NE\n");  
          comparison_op1 = (exprNode->dataType == INT_TYPE) ? "sub" : "feq.s";
          comparison_op2 = (exprNode->dataType == INT_TYPE) ? "snez" : "seqz";
          fprintf(write_file, "%s %s, %s, %s\n", comparison_op1, regName[rd], regName[rs], regName[rt]);
          fprintf(write_file, "%s %s, %s\n", comparison_op2, regName[rd], regName[rd]);
        } else if ( bin_op(exprNode) == BINARY_OP_GE ) {
          fprintf(stderr, "[gen_expr] EXPR_NODE - BINARY_OP_GE\n");  
          comparison_op1 = (exprNode->dataType == INT_TYPE) ? "slt" : "flt.s";
          fprintf(write_file, "%s %s, %s, %s\n", comparison_op1, regName[rd], regName[rs], regName[rt]);
          fprintf(write_file, "xori %s, %s, 1\n", regName[rd], regName[rd]);
        } else if ( bin_op(exprNode) == BINARY_OP_LE ) {
          fprintf(stderr, "[gen_expr] EXPR_NODE - BINARY_OP_LE\n");  
          comparison_op1 = (exprNode->dataType == INT_TYPE) ? "sgt" : "fgt.s";
          fprintf(write_file, "%s %s, %s, %s\n", comparison_op1, regName[rd], regName[rs], regName[rt]);
          fprintf(write_file, "xori %s, %s, 1\n", regName[rd], regName[rd]);
        } else if ( bin_op(exprNode) == BINARY_OP_GT ) {
          fprintf(stderr, "[gen_expr] EXPR_NODE - BINARY_OP_GT\n");  
          comparison_op1 = (exprNode->dataType == INT_TYPE) ? "sgt" : "fgt.s";
          fprintf(write_file, "%s %s, %s, %s\n", comparison_op1, regName[rd], regName[rs], regName[rt]);
        } else if ( bin_op(exprNode) == BINARY_OP_LT ) {
          fprintf(stderr, "[gen_expr] EXPR_NODE - BINARY_OP_LT\n");  
          comparison_op1 = (exprNode->dataType == INT_TYPE) ? "slt" : "flt.s";
          fprintf(write_file, "%s %s, %s, %s\n", comparison_op1, regName[rd], regName[rs], regName[rt]);
        } else {
          fprintf(write_file, "[gen_expr] receive unknown comparison\n");
        }
/*      swap rs and rd */
        int tmp = rs;
        rs = rd;
        rd = tmp;
/*      swap rs and rd */
      }
      free_reg(rt);
    } else if( expr_kind(exprNode) == UNARY_OPERATION ) {
      fprintf(stderr, "[gen_expr] EXPR_NODE - UNARY_OP\n");  
      float_or_not = exprNode->dataType == INT_TYPE ? "" : "f";
      rs = gen_expr(exprNode->child);
      if ( un_op(exprNode) == UNARY_OP_POSITIVE ) {
        fprintf(stderr, "[gen_expr] EXPR_NODE - UNARY_OP_POSITIVE\n"); 
      } else if ( un_op(exprNode) == UNARY_OP_NEGATIVE ) {
        fprintf(stderr, "[gen_expr] EXPR_NODE - UNARY_OP_NEGATIVE\n");  
        if ( exprNode->child->dataType == INT_TYPE ) {
          fprintf(write_file, "negw %s, %s\n", regName[rs], regName[rs]);  
        } else if ( exprNode->child->dataType == FLOAT_TYPE ) {
          fprintf(write_file, "fneg.s %s, %s\n", regName[rs], regName[rs]);
        } else {
          fprintf(write_file, "[gen_expr] unary->child has unknown dataType\n");
        }      
      } else if ( un_op(exprNode) == UNARY_OP_LOGICAL_NEGATION ) {
        if ( exprNode->dataType == FLOAT_TYPE ) {
          fprintf(stderr, "[gen_expr] exprNode UNARY_OP_LOGICAL_NEGATION is FLOAT_TYPE\n");
          exit(1);
        }
        fprintf(stderr, "[gen_expr] EXPR_NODE - UNARY_OP_LOGICAL_NEGATION\n");        
        fprintf(write_file, "seqz %s, %s\n", regName[rs], regName[rs]);
        fprintf(write_file, "andi %s, 0xff\n", regName[rs]);
      } else {
        fprintf(write_file, "[gen_expr] receive unknown comparison\n");
      }
    }
  } else if ( exprNode->nodeType == STMT_NODE ) { // solve statement
    fprintf(stderr, "[gen_expr] exprNode is STMT_NODE\n");
    gen_func(exprNode);
    if ( exprNode->dataType == INT_TYPE ) {
      rs = REG_T0;// t0
    } else if ( exprNode->dataType == FLOAT_TYPE ) {
      rs = REG_FT0;//ft0
    }
  } else if ( exprNode->nodeType == IDENTIFIER_NODE ) { // solve identifier
    if ( id_kind(exprNode) == ARRAY_ID ) { // TODO: solve array offset with gen_array_addr()
      fprintf(stderr, "[gen_expr] exprNode is IDENTIFIER_NODE - kind = ARRAY_ID\n");
      float_or_not = (exprNode->dataType == INT_TYPE) ? "" : "f";
      if ( exprNode->dataType == INT_TYPE ) {
        rs = get_int_reg(exprNode);
      } else if ( exprNode->dataType == FLOAT_TYPE ) {
        rs = get_float_reg(exprNode);
      } else {
        fprintf(stderr, "[gen_expr] IDENTIFIER_NODE - recieve unknown dataType exprNode\n");
        exit(1);
      }

      AST_NODE *dimListNode = exprNode->child;
      if ( dimListNode->nodeType == CONST_VALUE_NODE ) {
        if ( id_nest_level(exprNode) == 0 ) { // global arrays
          int rt = REG_S1; 
          fprintf(write_file, "lui %s, %%hi(_g_%s)\n", regName[rt], id_name(exprNode));
          fprintf(write_file, "addi %s, %s, %%lo(_g_%s)\n", regName[rt],regName[rt], id_name(exprNode));
          fprintf(write_file, "%slw %s, %d(%s)\n", float_or_not, regName[rs], 4*const_intval(dimListNode), regName[rt]);
        } else { // local array
            SymbolTableEntry *entry = id_entry(exprNode);
          fprintf(write_file, "%slw %s, -%d(fp)\n", float_or_not, regName[rs], 4*const_intval(dimListNode)+entry->offset);
        }
      } else {
        fprintf(stderr, "nani! the arrayNode->child is not CONST_VALUE_NODE\n");
        exit(1);
      }
    } else if ( id_kind(exprNode) == NORMAL_ID ) {
      if ( (rs=in_reg(exprNode)) < 0 ) {
        fprintf(stderr, "[gen_expr] varID: %s not in reg\n", id_name(exprNode));
        float_or_not = (exprNode->dataType == INT_TYPE) ? "" : "f";
        if ( exprNode->dataType == INT_TYPE ) {
          rs = get_int_reg(exprNode);
        } else if ( exprNode->dataType == FLOAT_TYPE ) {
          rs = get_float_reg(exprNode);
        } else {
          fprintf(stderr, "[gen_expr] IDENTIFIER_NODE - recieve unknown dataType exprNode\n");
          exit(1);
        }
        rt = get_int_reg(exprNode);
        if ( id_nest_level(exprNode) == 0 ) { // global variable
            fprintf(write_file, "la %s, _g_%s\n", regName[rt], id_name(exprNode));
            fprintf(write_file, "%slw %s, (%s)\n", float_or_not, regName[rs], regName[rt]);
            free_reg(rt);
        } else { // local varaible
            fprintf(write_file, "%slw %s, -%d(fp)\n", float_or_not, regName[rs], id_offset(exprNode));
        }
      } else {
        fprintf(stderr, "[gen_expr] varID: %s already in reg\n", id_name(exprNode));
      }
    }
  } else {
    fprintf(stderr, "[gen_expr] unknown node type\n");
    exit(1);
  }
  fprintf(stderr, "[gen_expr] end\n");
  return rs;
}

// block generation
void gen_block ( AST_NODE *blockNode ) {
  int frameSize = 0;
  FOR_ALL_CHILD(blockNode, child) {
    if ( child->nodeType == STMT_LIST_NODE ) {
      AST_NODE *stmtNode = child->child;
      FOR_ALL_CHILD(child, stmtNode) {
        gen_stmt(stmtNode);
      }
    }
  }
}

/* varaible declaration */
// static allocation
void gen_global_varDecl ( AST_NODE *varDeclDimList ) {
  fprintf(write_file, ".data\n");
  AST_NODE *declNode = varDeclDimList->child;
  FOR_ALL_CHILD(varDeclDimList, declNode) {
    if ( decl_kind(declNode) == VARIABLE_DECL ) {
      AST_NODE *typeNode = declNode->child;
      AST_NODE *idNode = typeNode->rightSibling;
      int ival;
      union ufloat fval;
      while ( idNode != NULL ) { // don't need to worry about char[] allocation (not in test input)
        ival = 0;
        fval.f = 0;
        SymbolTableEntry *sym = id_entry(idNode);
        char *name = id_name(idNode);
        TypeDescriptor* typeDesc = sym->attribute->attr.typeDescriptor;
        if ( id_kind(idNode) == WITH_INIT_ID ) {
          if ( const_type(idNode->child) == FLOATC ) {
            fval.f = const_fval(idNode->child);
          } else {
            ival = idNode->child->semantic_value.const1->const_u.intval;
          }
        }
        if ( typeDesc->kind == SCALAR_TYPE_DESCRIPTOR ) {
          if ( typeNode->dataType == INT_TYPE ) {
            fprintf(write_file, "_g_%s: .word %d\n", name, ival);
          } else if ( typeNode->dataType == FLOAT_TYPE ) {
            fprintf(write_file, "_g_%s: .word %u\n", name, fval.u);
          } else {
            fprintf(stderr, "[warning] recieve global declaration node neither INT_TYPE nor FLOAT_TYPE\n");
          }
        } else if ( typeDesc->kind == ARRAY_TYPE_DESCRIPTOR ) {
          int sz = 4;
          ArrayProperties arrayProp = typeDesc->properties.arrayProperties;
          for ( int i=0; i<arrayProp.dimension; ++i ) {
            sz *= arrayProp.sizeInEachDimension[i];
          }
          fprintf(write_file, "_g_%s: .zero %d\n", name, sz);
        }
        idNode = idNode->rightSibling;
      }
    }
  }
}

/* function related */ 
void gen_prologue ( char *func_name ) {
  fprintf(write_file, "sd ra,0(sp)\n");
  fprintf(write_file, "sd fp,-8(sp)\n");
  fprintf(write_file, "add fp,sp,-8\n");
  fprintf(write_file, "add sp,sp,-16\n");
  fprintf(write_file, "la ra,_frameSize_%s\n", func_name);
  fprintf(write_file, "lw ra,0(ra)\n");
  fprintf(write_file, "sub sp,sp,ra\n");
}

void caller_save() {
    for(int i = 0; i < REGISTER_NUM; i++){
        if(caller_save_list[i]){
            free_reg(i);
        }
    }
    return ;

}
void callee_save () {
  fprintf(write_file, "sd t0,8(sp)\n");
  fprintf(write_file, "sd t1,16(sp)\n");
  fprintf(write_file, "sd t2,24(sp)\n");
  fprintf(write_file, "sd t3,32(sp)\n");
  fprintf(write_file, "sd t4,40(sp)\n");
  fprintf(write_file, "sd t5,48(sp)\n");
  fprintf(write_file, "sd t6,56(sp)\n");
  fprintf(write_file, "sd s2,64(sp)\n");
  fprintf(write_file, "sd s3,72(sp)\n");
  fprintf(write_file, "sd s4,80(sp)\n");
  fprintf(write_file, "sd s5,88(sp)\n");
  fprintf(write_file, "sd s6,96(sp)\n");
  fprintf(write_file, "sd s7,104(sp)\n");
  fprintf(write_file, "sd s8,112(sp)\n");
  fprintf(write_file, "sd s9,120(sp)\n");
  fprintf(write_file, "sd s10,128(sp)\n");
  fprintf(write_file, "sd s11,136(sp)\n");
  fprintf(write_file, "sd fp,144(sp)\n");
  fprintf(write_file, "fsw ft0,152(sp)\n");
  fprintf(write_file, "fsw ft1,156(sp)\n");
  fprintf(write_file, "fsw ft2,160(sp)\n");
  fprintf(write_file, "fsw ft3,164(sp)\n");
  fprintf(write_file, "fsw ft4,168(sp)\n");
  fprintf(write_file, "fsw ft5,172(sp)\n");
  fprintf(write_file, "fsw ft6,176(sp)\n");
  fprintf(write_file, "fsw ft7,180(sp)\n");
}
// callee-restore
void callee_restore () {
  fprintf(write_file, "ld t0,8(sp)\n");
  fprintf(write_file, "ld t1,16(sp)\n");
  fprintf(write_file, "ld t2,24(sp)\n");
  fprintf(write_file, "ld t3,32(sp)\n");
  fprintf(write_file, "ld t4,40(sp)\n");
  fprintf(write_file, "ld t5,48(sp)\n");
  fprintf(write_file, "ld t6,56(sp)\n");
  fprintf(write_file, "ld s2,64(sp)\n");
  fprintf(write_file, "ld s3,72(sp)\n");
  fprintf(write_file, "ld s4,80(sp)\n");
  fprintf(write_file, "ld s5,88(sp)\n");
  fprintf(write_file, "ld s6,96(sp)\n");
  fprintf(write_file, "ld s7,104(sp)\n");
  fprintf(write_file, "ld s8,112(sp)\n");
  fprintf(write_file, "ld s9,120(sp)\n");
  fprintf(write_file, "ld s10,128(sp)\n");
  fprintf(write_file, "ld s11,136(sp)\n");
  fprintf(write_file, "ld fp,144(sp)\n");
  fprintf(write_file, "flw ft0,152(sp)\n");
  fprintf(write_file, "flw ft1,156(sp)\n");
  fprintf(write_file, "flw ft2,160(sp)\n");
  fprintf(write_file, "flw ft3,164(sp)\n");
  fprintf(write_file, "flw ft4,168(sp)\n");
  fprintf(write_file, "flw ft5,172(sp)\n");
  fprintf(write_file, "flw ft6,176(sp)\n");
  fprintf(write_file, "flw ft7,180(sp)\n");
}
void gen_epilogue (AST_NODE* funcDeclNode, char *func_name ) {
  fprintf(write_file, "ld ra,8(fp)\n");
  fprintf(write_file, "mv sp,fp\n");
  fprintf(write_file, "add sp,sp,8\n");
  fprintf(write_file, "ld fp,0(fp)\n");
  fprintf(write_file, "jr ra\n");
  fprintf(write_file, ".data\n");
  AST_NODE *idNode = funcDeclNode->child->rightSibling;
  SymbolTableEntry *entry = id_entry(idNode);
  fprintf(write_file, "_frameSize_%s: .word %d\n", func_name, 180+ entry->offset);
}

// stack allocatiom
void gen_funcDecl ( AST_NODE *funcDeclNode ) {
  AST_NODE *typeNode = funcDeclNode->child;
  AST_NODE *idNode = typeNode->rightSibling;
  char *func_name = id_name(idNode);
  AST_NODE *paramNode = idNode->rightSibling;
  AST_NODE *blockNode = paramNode->rightSibling;

  fprintf(stderr, "[gen_funcDecl] start\n");
  initial_reg();

  current_function = func_name;
  fprintf(write_file, ".text\n");
  // start of function
  fprintf(write_file, "_start_%s:\n", func_name);
  gen_prologue(func_name);
  callee_save();

  // go into blockNode
  gen_block(blockNode);

  // end of function
  fprintf(write_file, "_end_%s:\n", func_name);
  callee_restore();
  gen_epilogue(funcDeclNode, func_name);	

  fprintf(stderr, "[gen_funcDecl] end\n");
}

// function call
void gen_func ( AST_NODE *funcNode ) {
  fprintf(stderr, "[gen_func]\n");
  AST_NODE *idNode = funcNode->child;
  AST_NODE *paramListNode = idNode->rightSibling;

  char *func_name = id_name(idNode);

  if ( strcmp(func_name, "read") == 0 ) { // int read
    fprintf(stderr, "[gen_func] read\n");
    free_reg(REG_T0);
    fprintf(write_file, "call _read_int\n");
    fprintf(write_file, "mv t0, a0\n");
  } else if ( strcmp(func_name, "fread") == 0 ) { // float read
    fprintf(stderr, "[gen_func] fread\n");
    free_reg(REG_FT0);
    fprintf(write_file, "call _read_float\n");
    fprintf(write_file, "fmv.s ft0, fa0\n");
  } else if ( strcmp(func_name, "write") == 0 ) { // write( int / float / const char [])
    fprintf(stderr, "[gen_func] write\n"); 
    AST_NODE *paramNode = paramListNode->child;
    int reg;
    if ( paramNode->dataType == INT_TYPE ) {
      reg = gen_expr(paramNode);
      fprintf(stderr, "[gen_func] write int\n");
      fprintf(write_file, "mv a0, %s\n", regName[reg]);
      caller_save();
      fprintf(write_file, "jal _write_int\n");
    }
    if ( paramNode->dataType == FLOAT_TYPE ) {
      reg = gen_expr(paramNode);
      fprintf(stderr, "[gen_func] write float %d\n", reg);
      fprintf(write_file, "fmv.s fa0, %s\n", regName[reg]);
      caller_save();
      fprintf(write_file, "jal _write_float\n");
    }
    if ( paramNode->dataType == CONST_STRING_TYPE ) {
      free_reg(REG_T0);
      char str[256] = {};
      strncpy(str, paramNode->semantic_value.const1->const_u.sc+1, strlen(paramNode->semantic_value.const1->const_u.sc)-2);
      str[strlen(paramNode->semantic_value.const1->const_u.sc)-1] = '\0';

      fprintf(write_file, ".data\n");
      fprintf(write_file, "_CONSTANT_%d: .ascii \"%s\\000\"\n", constant_value_counter, str);
      fprintf(write_file, ".align 3\n");
      fprintf(write_file, ".text\n");
      fprintf(write_file, "la t0, _CONSTANT_%d\n", constant_value_counter);
      fprintf(write_file, "mv a0, t0\n");
      caller_save();
      fprintf(write_file, "jal _write_str\n");
      ++constant_value_counter;
    }
  } else { // normal function
    fprintf(write_file, "jal _start_%s\n", func_name);
  }
}