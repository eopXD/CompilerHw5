#ifndef __OFFSET_H__
#define __OFFSET_H__

#include "codeGen.h"

void offsetgen ( AST_NODE *program ); // mama call

int gen_offset ( AST_NODE *node, int offset ); // general node
int block_offset ( AST_NODE *blockNode, int offset ); // block
void param_offset ( AST_NODE *paramNode, int offset ); // parameter

#endif