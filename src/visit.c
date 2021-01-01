#include <exec/types.h>
#include <assert.h>
#include "keymaptable.h"
#include "visit.h"

// For each KCF_STRING/KCF_DEAD key, compute how many string/dead table entries are required.
//
// (Each key has one entry for a key press with no modifiers.  Each supported shift/alt/control
// modifier doubles the number of entries for a maximum of 8 entries.)
UBYTE calcNumEntries(UBYTE kmType) {
    UBYTE numEntries = 1;
    if (kmType & KCF_SHIFT)   { numEntries <<= 1; }
    if (kmType & KCF_ALT)     { numEntries <<= 1; }
    if (kmType & KCF_CONTROL) { numEntries <<= 1; }
    return numEntries;
}

void visitTable(void* pContext, UBYTE rawKey, UBYTE* pKmType, ULONG* pKmEntry, int mapSize, Visitor visitor) {
    const UBYTE* pKmStop = pKmType + mapSize;

    for (; pKmType < pKmStop; pKmType++, pKmEntry++, rawKey++) {
        const UBYTE type = *pKmType;
        
        switch (type >> 5) {
            case 0:
                visitor.pfnNormal(pContext, rawKey, type, *pKmEntry);
                break;
            case 1:
                assert(type & KCF_DEAD);
                visitor.pfnDead(pContext, rawKey, calcNumEntries(type), (UBYTE*) *pKmEntry);
                break;
            case 2:
                assert(type & KCF_STRING);
                visitor.pfnString(pContext, rawKey, calcNumEntries(type), (UBYTE*) *pKmEntry);
                break;
            default:
                assert(type == KCF_NOP);
                visitor.pfnNop(pContext, rawKey, type, *pKmEntry);
                break;
        }
    }
}

void visitLo(struct KeyMap* pKeyMap, void* pContext, Visitor visitor) {
    visitTable(
        pContext,
        /* rawKey: */ 0,
        pKeyMap->km_LoKeyMapTypes,
        pKeyMap->km_LoKeyMap,
        LO_MAP_LENGTH,
        visitor
    );
}

void visitHi(struct KeyMap* pKeyMap, void* pContext, Visitor visitor) {
    visitTable(
        pContext,
        /* rawKey: */ 0x40,
        pKeyMap->km_HiKeyMapTypes,
        pKeyMap->km_HiKeyMap,
        HI_MAP_LENGTH,
        visitor
    );
}

void visit(struct KeyMap* pKeyMap, void* pContext, Visitor visitor) {
    visitLo(pKeyMap, pContext, visitor);
    visitHi(pKeyMap, pContext, visitor);
}
