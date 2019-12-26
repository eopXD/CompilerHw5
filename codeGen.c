#include "header.h"
#include "codeGen.h"
#include "symbolTable.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FOR_ALL_CHILD(node, childnode) for(AST_NODE* childnode=node->child; childnode!=NULL; childnode=childnode->rightSibling)
#define DIRTY 1
#define CLEAN 0
#define FREE -1
#define BUSY -2
#define REG_FT0 32
#define REG_T0 5
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

int useRegList[64] = { 
    1, 1, 1, 1, 1,
    0, 0, 0, 1, 1,
    1, 1, 1, 1, 1,  
    1, 1, 0, 0, 0,
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

void copy_reg(RegTable a[], RegTable b[]){
    for(int i = 0; i < REGISTER_NUM; i ++){
        a[i] = b[i];
    }
    return ;
}
SymbolTableEntry* get_entry(AST_NODE* node)
{
   // retrieveSymbol(node->semantic_value.identifierSemanticValue.identifierName);
    return node->semantic_value.identifierSemanticValue.symbolTableEntry;
}
char* get_name(AST_NODE* node)
{
     return node->semantic_value.identifierSemanticValue.identifierName;
}

int in_reg(AST_NODE* node)
{
    if(node->nodeType == IDENTIFIER_NODE && node->semantic_value.identifierSemanticValue.kind == NORMAL_ID ){
      SymbolTableEntry* entry = get_entry(node);
      char*name = get_name(node);
      //printf("QQ, %s\n", name);
      if(entry->nestingLevel == 0){
          return -1;
      }
      for(int i = 0; i < REGISTER_NUM; i++){
          if(regTable[i].kind == VARIABLE_KIND){
              SymbolTableEntry* tmp = get_entry(regTable[i].node);
    //          printf("%s %s\n", entry->name,tmp->name);
              if(!strcmp(entry->name, tmp->name)){
                  return i;
              }
          }
      }
      return -1;
    }else if(node->nodeType == CONST_VALUE_NODE){
      if ( node->semantic_value.const1->const_type == INTEGERC ) { // const float
		for(int i = 0; i < REGISTER_NUM; i++){
          // TODO find the const table 
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
      
      for(int i = 32; i < REGISTER_NUM; i++){
          if(regTable[i].status == FREE){
              fprintf(stderr, "get free\n");
              store_reg(node, i);
              return i;
          }
      }
      for(int i = 32; i < REGISTER_NUM; i++){
          if(regTable[i].status == CLEAN){
              fprintf(stderr, "get clean\n");
              free_reg(i);
              store_reg(node, i);
              return i;
          }
      }
      for(int i = 32; i < REGISTER_NUM; i++){
          if(regTable[i].status == DIRTY && regTable[i].kind != TEMPORARY_KIND && regTable[i].kind != OTHER_KIND){
              //fprintf(stderr, "get dirty\n");
              free_reg(i);
              store_reg(node, i);
              return i;
          }
      }
      return -1;
}

int get_int_add_reg()
{
      int index = -1;
      for(int i = 0; i < 32; i++){
          if(regTable[i].status == FREE){
              store_reg(NULL, i);
              return i;
          }
      }
      for(int i = 0; i < 32; i++){
          if(regTable[i].status == CLEAN){
              free_reg(i);
              store_reg(NULL, i);
              return i;
          }
      }
      for(int i = 0; i < 32; i++){
          if(regTable[i].status == DIRTY && regTable[i].kind != TEMPORARY_KIND && regTable[i].kind != OTHER_KIND){
              free_reg(i);
              store_reg(NULL, i);
              return i;
          }
      }
      return -1;
}

int get_float_add_reg()
{
      int index = -1;
      for(int i = 32; i < REGISTER_NUM; i++){
          if(regTable[i].status == FREE){
              store_reg(NULL, i);
              return i;
          }
      }
      for(int i = 32; i < REGISTER_NUM; i++){
          if(regTable[i].status == CLEAN){
              free_reg(i);
              store_reg(NULL, i);
              return i;
          }
      }
      for(int i = 32; i < REGISTER_NUM; i++){
          if(regTable[i].status == DIRTY && regTable[i].kind != TEMPORARY_KIND && regTable[i].kind != OTHER_KIND){
    //          printf("%d %d\n",i,  regTable[i].status);
              free_reg(i);
              store_reg(NULL, i);
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
      
      for(int i = 0; i < 32; i++){
          if(regTable[i].status == FREE){
              store_reg(node, i);
              return i;
          }
      }
      for(int i = 0; i < 32; i++){
          if(regTable[i].status == CLEAN){
              free_reg(i);
              store_reg(node, i);
              return i;
          }
      }
      for(int i = 0; i < 32; i++){
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
    if ( reg.node->nodeType == IDENTIFIER_NODE ) {
      regTable[regIndex].status = BUSY;
	  AST_NODE *node = reg.node;
      SymbolTableEntry* entry = get_entry(reg.node);
      float_or_not = reg.node->dataType == INT_TYPE ? "" : "f";
      //addr_reg = reg.node->dataType == INT_TYPE ? 9 : 40;
      addr_reg = 9; 
	  fprintf(stderr, "[free_reg] writing data for var %s\n", reg.node->semantic_value.identifierSemanticValue.identifierName);
      fprintf(stderr, "[free_reg] (regIndex, addr_reg): (%d %d)\n", regIndex, addr_reg);
      if ( node->semantic_value.identifierSemanticValue.kind == NORMAL_ID ) { // normal var
        if ( entry->nestingLevel == 0 ) { // global normal
          //fprintf(write_file, "%sla %s, _g_%s\n", float_or_not, regName[addr_reg], node->semantic_value.identifierSemanticValue.identifierName);
          fprintf(write_file, "la %s, _g_%s\n", regName[addr_reg], node->semantic_value.identifierSemanticValue.identifierName);
          fprintf(write_file, "%ssw %s, 0(%s)\n", float_or_not, regName[regIndex], regName[addr_reg]);
        } else { // local normal
            //TODO local no need to ?
          //fprintf(write_file, "%sla %s, -%d(fp)\n", float_or_not, regName[addr_reg], 4*node->semantic_value.identifierSemanticValue.symbolTableEntry->offset);
          //fprintf(write_file, "%ssw, %s, %s\n", float_or_not, regName[regIndex], regName[addr_reg]);    
          if(!strcmp(float_or_not,"f")&& regIndex < 32){
              printf("QQQQ\n");
              exit(0);
          }
          fprintf(write_file, "%ssw %s, -%d(fp)\n", float_or_not, regName[regIndex], node->semantic_value.identifierSemanticValue.symbolTableEntry->offset);
        }
      } else if ( node->semantic_value.identifierSemanticValue.kind == ARRAY_ID ) { // array var
        if ( node->semantic_value.identifierSemanticValue.symbolTableEntry->nestingLevel == 0 ) { // global array
          fprintf(write_file, "lui %s, %%hi(_g_%s)\n", regName[addr_reg], node->semantic_value.identifierSemanticValue.identifierName);
          fprintf(write_file, "addi %s, %s, %%lo(_g_%s)\n", regName[addr_reg], regName[addr_reg], node->semantic_value.identifierSemanticValue.identifierName);
          fprintf(write_file, "%ssw %s, %d(%s)\n", float_or_not, regName[regIndex], 4*node->child->semantic_value.const1->const_u.intval, regName[addr_reg]);
        } else { // local array
          if(!strcmp(float_or_not,"f")&& regIndex < 32){
              printf("QQQQ\n");
              exit(0);
          }
          fprintf(write_file, "%ssw %s, -%d(fp)\n", float_or_not, regName[regIndex], 4*node->semantic_value.identifierSemanticValue.symbolTableEntry->offset);
        }
      } else {
        fprintf(stderr, "[free_reg] receive bad node SemanticValueKind\n");
        exit(1);
      }
    }
  }
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
          if(stmtNode->semantic_value.stmtSemanticValue.kind == FUNCTION_CALL_STMT){
              gen_func(stmtNode);
          }else{

              switch(stmtNode->semantic_value.stmtSemanticValue.kind){
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
  if(lhs->nodeType == IDENTIFIER_NODE && lhs->semantic_value.identifierSemanticValue.kind == NORMAL_ID ){
      SymbolTableEntry* entry = get_entry(lhs);
      if(entry->nestingLevel == 0){
          free_reg(rhs_reg);
      }
  }
}

void gen_returnStmt(AST_NODE* returnNode)
{
      int index = gen_expr(returnNode->child);
      SymbolTableEntry* entry = get_entry(returnNode->child);
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
          gen_stmt(ifBodyNode);
          fprintf(write_file, "_Lexit%d\n", local_label_number);
  
      }else{
          fprintf(write_file, "beqz %s, _Lelse%d  \n", regName[index], local_label_number);
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
    int index = gen_expr(boolExpression);
    fprintf(write_file, "beqz %s, _Lexit%d\n", regName[index], local_label_number);
    gen_stmt(boolExpression->rightSibling);
    fprintf(write_file, "j _Test%d\n", local_label_number);
    fprintf(write_file, "_Lexit%d\n", local_label_number);
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


// returns a register that has the correct value of the array address
/*
BEWARE, because testdata only contains 1D array with constant index, so right here I am only fetching for the constant integer value inside
So this part is currently ommited because hw5 does not support pointer or multiple dimension array.
*/
/*
int gen_array_addr ( AST_NODE *idNode ) {
	ArrayProperties arrayProp = 
	 idNode->semantic_value.identifierSemanticValue.symbolTableEntry->attribute->attr.typeDescriptor->properties.arrayProperties;
	AST_NODE *dimListNode = idNode->child; // 
	int rs = get_int_reg(idNode);
  int offset_cnt = 0; // offset count

  fprintf(write_file, "li %s, 0\n", regName[offset_reg]); // set rt to 0
	for ( int i=0; i<arrayProp.dimension; ++i ) {
		int dim_reg = gen_expr(dimNode); // dimension register
		int sz = (i == arrayProp.dimension-1) ? 4 : arrayProp.sizeInEachDimension[i];
		
    fprintf(write_file, "add %s, %s, %s\n", regName[ofst], regName[ofst], regName[dim_reg]);
    fprintf(write_file, "li %s, %d\n", regName[multply], sz);
    fprintf(write_file, "mul %s, %s, %s\n", regName[ofst], regName[ofst], regName[mult]);

		dimNode = dimNode->rightSibling;
		free_reg(dim_reg);
	}

  int rs = get_int_reg(idNode);

	if ( idNode->semantic_value.identifierSemanticValue.symbolTableEntry->nestingLevel == 0 ) { // global array
		fprintf(write_file, "la %s, _g_%s\n", regName[rd], idNode->semantic_value.identifierSemanticValue.identifierName);

		fprintf(write_file, "lw %s, %s(%s)\n", regName[rs], regName[rt], regName[rd]);
		free_reg(rd);
	} else { // local array
		fprintf(write_file, "lw %s, -%d(fp)\n", regName[rs], idNode->semantic_value.identifierSemanticValue.symbolTableEntry->offset);
	}
	free_reg(rt);
	return (rs);
}*/


// expression returns register for use
int gen_expr ( AST_NODE *exprNode ) {
	fprintf(stderr, "[gen_expr] start\n");

// results put into 'rs'
	int rs, rt;
	char *float_or_not;

	int ival;
	float fval;

	union ufloat uf;
	char str[256] = {};

	if ( exprNode->nodeType == CONST_VALUE_NODE ) { // constant value
		fprintf(stderr, "[gen_expr] CONST_VALUE_NODE\n");
		if ( exprNode->semantic_value.const1->const_type == INTEGERC ) { // const integer
			if ( (rs=in_reg(exprNode)) < 0 ) {
				rs = get_int_reg(exprNode);
				fprintf(write_file, ".data\n");
				fprintf(write_file, "_CONSTANT_%d: .word %d\n", constant_value_counter, exprNode->semantic_value.const1->const_u.intval);
				fprintf(write_file, ".align 3\n");
				fprintf(write_file, ".text\n");
				fprintf(write_file, "lw %s, _CONSTANT_%d\n", regName[rs], constant_value_counter);
				++constant_value_counter;
			}
		} else if ( exprNode->semantic_value.const1->const_type == FLOATC ) { // const float
			fprintf(stderr, "CONST_VALUE_NODE: FLOATC\n");
			if ( (rs=in_reg(exprNode)) < 0 ) {
				fprintf(stderr, "value is not in reg\n");
				uf.f = exprNode->semantic_value.const1->const_u.fval;
				rs = get_float_reg(exprNode);
				rt = get_int_reg(exprNode);
                fprintf(stderr ,"%d %d\n", rs, rt);
				fprintf(stderr, "get reg for rs and rt\n");
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
			}
			free_reg(rt);
			fprintf(stderr, "free reg for rt\n");
		} else if ( exprNode->semantic_value.const1->const_type == STRINGC ) { // const string
			if ( (rs = in_reg(exprNode)) < 0 ) {
				rs = get_int_reg(exprNode);
				//memcpy(str, exprNode->semantic_value.const1->const_u.sc, strlen(exprNode->semantic_value.const1->const_u.sc));
				strncpy(str, exprNode->semantic_value.const1->const_u.sc+1, strlen(exprNode->semantic_value.const1->const_u.sc)-2);
                str[strlen(exprNode->semantic_value.const1->const_u.sc)-1] = '\0';
				fprintf(write_file, ".data\n");
				fprintf(write_file, "_CONSTANT_%d:\n", constant_value_counter);
                //printf("%s\n", str);
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
		if( exprNode->dataType == INT_TYPE ) {
			rs = get_int_reg(exprNode);
			fprintf(write_file, "li %s, %d\n", regName[rs], exprNode->semantic_value.exprSemanticValue.constEvalValue.iValue);
		} else if( exprNode->semantic_value.exprSemanticValue.kind == BINARY_OPERATION ) {
			float_or_not = exprNode->dataType == INT_TYPE ? "" : "f";
			rs = gen_expr(exprNode->child);
			rt = gen_expr(exprNode->child->rightSibling);

			switch ( exprNode->semantic_value.exprSemanticValue.op.binaryOp ) {
				case BINARY_OP_ADD: 
					fprintf(write_file, "%s add %s, %s, %s\n", float_or_not, regName[rs], regName[rs], regName[rt]); break;
				case BINARY_OP_SUB: 
					fprintf(write_file, "%s sub %s, %s, %s\n", float_or_not, regName[rs], regName[rs], regName[rt]); break;
				case BINARY_OP_MUL: 
					fprintf(write_file, "%s mul %s, %s, %s\n", float_or_not, regName[rs], regName[rs], regName[rt]); break;
				case BINARY_OP_DIV: 
					fprintf(write_file, "%s div %s, %s, %s\n", float_or_not, regName[rs], regName[rs], regName[rt]); break;
				case BINARY_OP_AND: 
					fprintf(write_file, "%s and %s, %s, %s\n", float_or_not, regName[rs], regName[rs], regName[rt]); break;
				case BINARY_OP_OR: 
                    	fprintf(write_file, "%sor %s, %s, %s\n", float_or_not, regName[rs], regName[rs], regName[rt]); break;
				case BINARY_OP_EQ:
					fprintf(write_file, "sub %s, %s, %s\n", regName[rs], regName[rs], regName[rt]);
					fprintf(write_file, "seqz %s, %s\n", regName[rs], regName[rs]); break;
				case BINARY_OP_NE:
					fprintf(write_file, "sub %s, %s, %s\n", regName[rs], regName[rs], regName[rt]);
					fprintf(write_file, "snez %s, %s\n", regName[rs], regName[rs]); break;
				case BINARY_OP_GE:
					fprintf(write_file, "slt %s, %s, %s\n", regName[rs], regName[rs], regName[rt]);
					fprintf(write_file, "xori %s, %s, 1\n", regName[rs], regName[rs]); break;
				case BINARY_OP_LE:
					fprintf(write_file, "sgt %s, %s, %s\n", regName[rs], regName[rs], regName[rt]);
					fprintf(write_file, "xori %s, %s, 1\n", regName[rs], regName[rs]); break;
				case BINARY_OP_GT:
					fprintf(write_file, "sgt %s, %s, %s\n", regName[rs], regName[rs], regName[rt]); break;
				case BINARY_OP_LT:
					fprintf(write_file, "slt %s, %s, %s\n", regName[rs], regName[rs], regName[rt]);	break;
				default: break;
			}
			free_reg(rt);
		} else if( exprNode->semantic_value.exprSemanticValue.kind == UNARY_OPERATION ) {
			float_or_not = exprNode->dataType == INT_TYPE ? "" : "f";
			rs = gen_expr(exprNode->child);
			switch ( exprNode->semantic_value.exprSemanticValue.op.unaryOp ) {
				case UNARY_OP_POSITIVE: 
					/* don't need to do anything */
					break;
				case UNARY_OP_NEGATIVE: 
					if ( exprNode->child->dataType == INT_TYPE ) {
						fprintf(write_file, "negw %s, %s\n", regName[rs], regName[rs]);
					} else if ( exprNode->child->dataType == FLOAT_TYPE ) {
						fprintf(write_file, "fneg.s %s, %s\n", regName[rs], regName[rs]);
					} else {
						fprintf(write_file, "[gen_expr] unary->child has unknown dataType\n");
					}
				case UNARY_OP_LOGICAL_NEGATION:
          fprintf(write_file, "andi %s, 0xff\n", regName[rs]);
					break;
				default: break;
			}

		}
	} else if ( exprNode->nodeType == STMT_NODE ) { // solve statement
		gen_func(exprNode);
		if ( exprNode->dataType == INT_TYPE ) {
			rs = REG_T0;// t0
		} else if ( exprNode->dataType == FLOAT_TYPE ) {
			rs = REG_FT0;//ft0
		}
	} else if ( exprNode->nodeType == IDENTIFIER_NODE ) { // solve identifier
    if ( exprNode->semantic_value.identifierSemanticValue.kind == ARRAY_ID ) {
    /*
    testdata in hw5 shall not do into this if-statement
    (BEWARE of variable type)
    */  
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
        if ( exprNode->semantic_value.identifierSemanticValue.symbolTableEntry->nestingLevel == 0 ) { // global arrays
          int rt = 9; 
          //fprintf(write_file, "lui %s, %%hi(_g_%s)\n", regName[rs], exprNode->semantic_value.identifierSemanticValue.identifierName);
          fprintf(write_file, "lui %s, %%hi(_g_%s)\n", regName[rt], exprNode->semantic_value.identifierSemanticValue.identifierName);
          fprintf(write_file, "addi %s, %s, %%lo(_g_%s)\n", regName[rt],regName[rt], exprNode->semantic_value.identifierSemanticValue.identifierName);
          fprintf(write_file, "%slw %s, %d(%s)\n", float_or_not, regName[rs], 4*dimListNode->semantic_value.const1->const_u.intval, regName[rt]);
        } else { // local array
          fprintf(write_file, "%slw %s, -%d(fp)\n", float_or_not, regName[rs], 4*exprNode->semantic_value.identifierSemanticValue.symbolTableEntry->offset);
        }
      } else {
        fprintf(stderr, "nani! the arrayNode->child is not CONST_VALUE_NODE\n");
        exit(1);
      }
		} else if ( exprNode->semantic_value.identifierSemanticValue.kind == NORMAL_ID ) {
			if ( (rs=in_reg(exprNode)) < 0 ) {
        fprintf(stderr, "[gen_expr] varID: %s not in reg\n", get_name(exprNode));
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
        if ( exprNode->semantic_value.identifierSemanticValue.symbolTableEntry->nestingLevel == 0 ) { // global variable
            fprintf(write_file, "la %s, _g_%s\n", regName[rt], exprNode->semantic_value.identifierSemanticValue.identifierName);
            fprintf(write_file, "%slw %s, (%s)\n", float_or_not, regName[rs], regName[rt]);
            free_reg(rt);
        } else { // local varaible
            fprintf(write_file, "%slw %s, -%d(fp)\n", float_or_not, regName[rs], exprNode->semantic_value.identifierSemanticValue.symbolTableEntry->offset);
        }
			} else {
        fprintf(stderr, "[gen_expr] varID: %s already in reg\n", get_name(exprNode));
      }
		}
	} else {
		fprintf(stderr, "[gen_expr] unknown node type\n");
		exit(1);
	}
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
		if ( declNode->semantic_value.declSemanticValue.kind == VARIABLE_DECL ) {
			AST_NODE *typeNode = declNode->child;
			AST_NODE *idNode = typeNode->rightSibling;
			int ival;
			union ufloat fval;
      while ( idNode != NULL ) { // don't need to worry about char[] allocation (not in test input)
        ival = 0;
        fval.f = 0;
				SymbolTableEntry *sym = idNode->semantic_value.identifierSemanticValue.symbolTableEntry;
				char *name = idNode->semantic_value.identifierSemanticValue.identifierName;
				TypeDescriptor* typeDesc = sym->attribute->attr.typeDescriptor;
        if ( idNode->semantic_value.identifierSemanticValue.kind == WITH_INIT_ID ) {
          if ( idNode->child->semantic_value.const1->const_type == FLOATC ) {
            fval.f = idNode->child->semantic_value.const1->const_u.fval;
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
    SymbolTableEntry* entry = get_entry(idNode);
    /*if(entry == NULL){
        printf("QQQQQ\n");
    }*/
	fprintf(write_file, "_frameSize_%s: .word %d\n", func_name, 180+ entry->offset);
}

// stack allocatiom
void gen_funcDecl ( AST_NODE *funcDeclNode ) {
	AST_NODE *typeNode = funcDeclNode->child;
	AST_NODE *idNode = typeNode->rightSibling;
	char *func_name = idNode->semantic_value.identifierSemanticValue.identifierName;
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

	char *func_name = idNode->semantic_value.identifierSemanticValue.identifierName;

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
			fprintf(write_file, "jal _write_int\n");
		}
		if ( paramNode->dataType == FLOAT_TYPE ) {
	        reg = gen_expr(paramNode);
            fprintf(stderr, "[gen_func] write float %d\n", reg);
            fprintf(write_file, "fmv.s fa0, %s\n", regName[reg]);
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
			fprintf(write_file, "jal _write_str\n");
			++constant_value_counter;
		}
	} else { // normal function
		fprintf(write_file, "jal jal _start_%s\n", func_name);
	}
}


/* offset analysis */

void offsetgen ( AST_NODE *program ) {
	gen_offset(program, 0);
}
int gen_offset ( AST_NODE *node, int offset ) {
	if (node->nodeType==DECLARATION_NODE && node->semantic_value.declSemanticValue.kind == FUNCTION_DECL ) {
		offset = 0;
	} else if ( node->nodeType == BLOCK_NODE ) {
		offset = block_offset(node, offset);
	} else if ( node->nodeType == PARAM_LIST_NODE ) {
		param_offset(node, offset);
		return (offset);
	}
	FOR_ALL_CHILD(node, child) {
		int x = gen_offset(child, offset);
		if ( x < offset ) {
			offset = x;
		}
	}
	if ( node->nodeType==DECLARATION_NODE && node->semantic_value.declSemanticValue.kind == FUNCTION_DECL ) {
		AST_NODE *idNode = node->child->rightSibling;
		idNode->semantic_value.identifierSemanticValue.symbolTableEntry = 
		 retrieveSymbol(idNode->semantic_value.identifierSemanticValue.identifierName);
		idNode->semantic_value.identifierSemanticValue.symbolTableEntry->offset = offset;
	}
	return (offset);
}
void param_offset ( AST_NODE *paramNode, int offset ) {
	offset += 16;
	FOR_ALL_CHILD(paramNode, child) {
		child->semantic_value.identifierSemanticValue.symbolTableEntry->offset = offset;
		offset += 4;
	}
}
int block_offset ( AST_NODE *blockNode, int offset ) {
	if ( blockNode == NULL ) {
		return (offset);
	}
	if ( blockNode->child->nodeType != VARIABLE_DECL_LIST_NODE ) {
		return (offset);
	}

	AST_NODE *declList = blockNode->child;
	FOR_ALL_CHILD(declList, declNode) {
		AST_NODE *idNode = declNode->child->rightSibling;
		while ( idNode != NULL ) {
			SymbolTableEntry *sym = idNode->semantic_value.identifierSemanticValue.symbolTableEntry;
			if (sym->attribute->attr.typeDescriptor->kind == ARRAY_TYPE_DESCRIPTOR ) {
				ArrayProperties arrayProp = sym->attribute->attr.typeDescriptor->properties.arrayProperties;
				int sz = 4;
				for ( int i=0; i< arrayProp.dimension; ++i ) {
					sz *= arrayProp.sizeInEachDimension[i];
				}
				offset += sz;
			} else {
				offset += 4;
			}
			sym->offset = offset;
			idNode = idNode->rightSibling;
		}
	}
	return (offset);
}
