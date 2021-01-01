#ifndef VISIT_H
#define VISIT_H

#include <exec/types.h>
#include <proto/keymap.h>

typedef struct {
    void (*pfnNormal)(void* pContext, UBYTE rawKey, UBYTE kmType, ULONG kmEntry);
    void (*pfnString)(void* pContext, UBYTE rawKey, int entryCount, const UBYTE* pStringDesc);
    void (*pfnDead)(void* pContext, UBYTE rawKey, int entryCount, const UBYTE* pDeadDesc);
    void (*pfnNop)(void* pContext, UBYTE rawKey, UBYTE kmType, ULONG kmEntry);
} Visitor;

void visit(struct KeyMap* pKeyMap, void* pContext, Visitor visitor);
void visitLo(struct KeyMap* pKeyMap, void* pContext, Visitor visitor);
void visitHi(struct KeyMap* pKeyMap, void* pContext, Visitor visitor);

#endif