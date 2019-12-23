#include "header.h"
#include "codeGen.h"
#include "symbolTable.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FOR_ALL_CHILD(node, childnode) for(AST_NODE* childnode=node->child; childnode!=NULL; childnode=childnode->rightSibling)


FILE *write_file;
int constant_value_counter; // for constant allocation
char *current_function; // current function name

void codegen ( AST_NODE *program ) {
	write_file = fopen("output.S", "w");
	if ( write_file == NULL ) {
		fprintf(stderr, "open write file fail\n");
		exit(1);
	}
	constant_value_counter = 1;

	FOR_ALL_CHILD(program, child) {
		if ( child->nodeType == VARIABLE_DECL_LIST_NODE ) { // global decl
			gen_global_varDecl(child);
		} else if ( child->nodeType == DECLARATION_NODE ) { // global function (including main)
			gen_funcDecl(child);
		}
	}
	fclose(write_file);
}

// block generation
void gen_block ( AST_NODE *blockNode ) {
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

/* varaible declaration */
// static allocation
void gen_global_varDecl ( AST_NODE *varDeclDimList ) {
	fprintf(write_file, ".data\n");
	AST_NODE *declNode = varDeclDimList->child;
	FOR_ALL_CHILD(varDeclDimList, declNode) {
		if ( declNode->semantic_value.declSemanticValue.kind == VARIABLE_DECL ) {
			AST_NODE *typeNode = declNode->child;
			AST_NODE *idNode = typeNode->rightSibling;
			int ival =0;
			float fval = 0;
			while ( idNode != NULL ) { // don't need to worry about char[] allocation (not in test input)
				SymbolTableEntry *sym = idNode->semantic_value.identifierSemanticValue.symbolTableEntry;
				char *name = idNode->semantic_value.identifierSemanticValue.identifierName;
				TypeDescriptor typeDesc = sym->attribute->attr.typeDescriptor;
				
				// global var in test data don't have initial value
				// TODO: fetch const_val to ival/fval


				// TODO: confirm correct directives	
				if ( typeDesc->kind == SCALAR_TYPE_DESCRIPTOR ) {
					if ( typeNode->dataType == INT_TYPE ) {
						fprintf(write_file, "_g_%s .DIRECTIVE %d\n", name, ival);
					} else if ( typeNode->dataType == FLOAT_TYPE ) {
						fprintf(write_file, "_g_%s .DIRECTIVE %f\n", name, fval);
					} else {
						fprintf(stderr, "[warning] recieve global declaration node neither INT_TYPE nor FLOAT_TYPE\n");

					}
				} else if ( typeDesc->kind == ARRAY_TYPE_DESCRIPTOR ) {
					int sz = 4;
					ArrayProperties arrayProp = typeDesc->properties.arrayProperties;
					for ( int i=0; i<arrayProp.dimension; ++i ) {
						sz *= arrayProp.sizeInEachDimension[i];
					}
					fprintf(write_file, "_g_%s .DIRECTIVE %d\n", name, sz);
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
void gen_epilogue ( char *func_name ) {
	fprintf(write_file, "ld ra,8(fp)\n");
	fprintf(write_file, "mv sp,fp\n");
	fprintf(write_file, "add sp,sp,8\n");
	fprintf(write_file, "ld fp,0(fp)\n");
	fprintf(write_file, "jr ra\n");
	fprintf(write_file, ".data\n");
// TODO: offset to be determined
	fprintf(write_file, "_frameSize_%s: .word %d\n", func_name, OFFSET);
}

// stack allocatiom
void gen_funcDecl ( AST_NODE *funcDeclNode ) {
	AST_NODE *typeNode = funcDeclNode->child;
	AST_NODE *idNode = typeNode->rightSibling;
	char *func_name = idNode->semantic_value.identifierSemanticValue.identifierName;
	AST_NODE *paramNode = idNode->rightSibling;
	AST_NODE *blockNode = paramNode->rightSibling;

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
	gen_epilogue(func_name);	
}

// function call
void gen_func ( AST_NODE *funcNode ) {
	AST_NODE *idNode = funcNode->child;
	AST_NODE *paramListNode = idNode->rightSibling;

	char *func_name = idNode->semantic_value.identifierSemanticValue.identifierName;

	if ( strcmp(func_name, "read") == 0 ) { // int read
		fprintf(write_file, "call _read_int\n");
		fprintf(write_file, "mv t0, a0\n");
		fprintf(write_file, "str t0, -4(fp)\n")
	} else if ( strcmp(func_name, "fread") == 0 ) { // float read
		fprintf(write_file, "call _read_float\n");
		fprintf(write_file, "fmv.s ft0, fa0\n");
		fprintf(write_file, "str ft0, -8(fp)\n");
	} else if ( strcmp(func_name, "write") == 0 ) { // write( int / float / const char [])
		AST_NODE *paramNode = paramListNode->child;
		char *reg = gen_expr(paramNode);
		if ( paramNode->dataType == INT_TYPE ) {
			fprintf(write_file, "lw t0, -4(fp)\n");
			fprintf(write_file, "mv %s, t0\n", reg);
			fprintf(write_file, "jal _write_int\n");
		}
		if ( paramNode->dataType == FLOAT_TYPE ) {
			fprintf(write_file, "lw ft0, -8(fp)\n");
			fprintf(write_file, "fmv.s %s, ft0\n", reg);
			fprintf(write_file, "jal _write_float\n");
		}
		if ( paramNode->dataType == CONST_STRING_TYPE ) {
			fprintf(write_file, ".data\n");
			fprintf(write_file, "_CONSTANT_%d: \"%s\"\\000", constant_value_counter, reg);
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

void gen_offset ( AST_NODE *program ) {
	gen_offset(program, 0);
}
int gen_offset ( AST_NODE *node, int offset ) {
	if ( node->nodeType==DECLARATION_NODE && node->semantic_value.declSemanticValue.kind = FUNCTION_DECL ) {
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
	if ( node->nodeType==DECLARATION_NODE && node->semantic_value.declSemanticValue.kind = FUNCTION_DECL ) {
		AST_NODE *idNode = node->child->rightSibling;
		idNode->semantic_value.identifierSemanticValue.symbolTableEntry = 
		 retrieveSymbol(idNode->semantic_value.identifierSemanticValue.identifierName);
		idNode->semantic_value.identifierSemanticValue.symbolTableEntry->offset = offset;
	}
	return (offset);
}
void param_offset ( AST_NODE *paramNode, int offset ) {
	offset += 16;
	FOR_ALL_CHILD(param_node, child) {
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
			if ( sym->attribute->attr.typeDescriptor == ARRAY_TYPE_DESCRIPTOR ) {
				ArrayProperties arrayProp = sym->attribute.typeDescriptor->properties.arrayProperties;
				int sz = 4;
				for ( int i=0; i<properties.dimension; ++i ) {
					sz *= properties.sizeInEachDimension[i];
				}
				offset -= sz;
			} else {
				offset -= 4;
			}
			sym->offset = offset;
			idNode = idNode->rightSibling;
		}
	}
	return (offset);
}