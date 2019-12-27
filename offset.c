#include "offset.h"

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
		if ( x > offset ) {
			offset = x;
		}
	}
	if ( node->nodeType==DECLARATION_NODE && node->semantic_value.declSemanticValue.kind == FUNCTION_DECL ) {
		AST_NODE *idNode = node->child->rightSibling;
		idNode->semantic_value.identifierSemanticValue.symbolTableEntry = 
		 retrieveSymbol(idNode->semantic_value.identifierSemanticValue.identifierName);
        //AST_NODE* blocknode = idNode->rightSibling->rightSibling; 
        //SymbolTableEntry* entry = get_entry(blocknode);
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
				int sz = 1;
				for ( int i=0; i< arrayProp.dimension; ++i ) {
					sz *= arrayProp.sizeInEachDimension[i];
				}
				fprintf(stderr, "[block_offset] array of size %d, ", sz);
				sz *= 4;
				offset += sz;
				fprintf(stderr, "idNode %s: %d\n", 
				 idNode->semantic_value.identifierSemanticValue.identifierName, offset);
			} else{
				// int/float takes 4 byte (single word), not doing double word here
                //offset += (sym->attribute->attr.typeDescriptor->properties.dataType == INT_TYPE) ? 4 : 4;
				offset += 4;
				fprintf(stderr, "[block_offset] non-array, idNode %s: %d\n", 
				 idNode->semantic_value.identifierSemanticValue.identifierName, offset);
            }
			sym->offset = offset;
			idNode = idNode->rightSibling;
		}
	}
	return (offset);
}