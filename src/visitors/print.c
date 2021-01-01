#include "print.h"
#include "measure.h"
#include "../keymaptable.h"
#include <exec/types.h>
#include <proto/keymap.h>
#include <stdio.h>
#include <assert.h>

BOOL isPrintable(UBYTE ch) {
    return ((0x1F < ch) && (ch < 0x7F));     // ASCII printable (excludes 7F = DEL)
}

void printChar(char ch) {
    if (isPrintable((UBYTE) ch)) {
        printf("%c", ch);
    } else {
        printf("{0x%x}", (UBYTE) ch);
    }
}

void printChars(char* pCh, UBYTE len) {
    printf("'");
    for (int i = 0; i < len; i++) {
        printChar(*(pCh++));
    }
    printf("' ");
}

void printNormal(void* pContext, UBYTE rawKey, UBYTE kmType, ULONG kmEntry) {
    printf("%x: '", rawKey);
    printChar((char) kmEntry);
    printChar((char) (kmEntry >> 8));
    printChar((char) (kmEntry >> 16));
    printChar((char) (kmEntry >> 24));
    printf("'\n");
}

void printNop(void* pContext, UBYTE rawKey, UBYTE kmType, ULONG kmEntry) {
    printf("%x: (Nop) %lx\n", rawKey, kmEntry);
}

void printString(void* pContext, UBYTE rawKey, int numEntries, const UBYTE* pKmEntry) {
    printf("%x: (String n=%d) ", rawKey, numEntries);
    const UBYTE* pDescStart = pKmEntry;
    for (int n = 0; n < numEntries; n++) {
        UBYTE len = *(pKmEntry++);
        char* pCh = (char*)(pDescStart + *(pKmEntry++));
        printf("len=%u ", len);
        printChars(pCh, len);
    }
    printf("\n");
}

void printDead(void* pContext, UBYTE rawKey, int numEntries, const UBYTE* pTable) {
    const UBYTE* pStart = pTable;
    printf("%x: (DEAD @%p, n=%d) ", rawKey, (void*) pStart, numEntries);
    for (int n = 0; n < numEntries; n++) {
        const UBYTE kind = *(pTable++);
        switch (kind) {
            case 0:
                printf("NONE '");
                printChar(*(pTable++));
                printf("' ");
                break;
            case DPF_DEAD:
                printf("DEAD %u ", *(pTable++));
                break;
            default:
                assert(kind == DPF_MOD);
                const UBYTE offset = *(pTable++);
                printf("MOD +%u ", offset);
                if (pContext != NULL) {
                    const Sizes* pSizes = (Sizes*) pContext;
                    char* pCh = (char*) pStart + offset;
                    printChars(pCh, pSizes->deadCharTableBytes);
                }
                break;
        }
    }
    printf("\n");
}

void printKeymapAddresses(struct KeyMap* pKeymap) {
    printf("km ptr: %p\n", (void *) pKeymap);
    printf("km_LoKeyMapTypes: %p-%p\n", (void*) pKeymap->km_LoKeyMapTypes, (void*)(((UBYTE*) pKeymap->km_LoKeyMapTypes) + LO_TYPE_LENGTH));
    printf("km_LoKeyMap:      %p-%p\n", (void*) pKeymap->km_LoKeyMap, (void*)(((UBYTE*) pKeymap->km_LoKeyMap) + LO_TYPE_LENGTH * sizeof(ULONG)));
    printf("km_LoCapsable:    %p-%p\n", (void*) pKeymap->km_LoCapsable, (void*)(((UBYTE*) pKeymap->km_LoCapsable) + LO_CAPS_BYTE_SIZE));
    printf("km_LoRepeatable:  %p-%p\n", (void*) pKeymap->km_LoRepeatable, (void*)(((UBYTE*) pKeymap->km_LoRepeatable) + LO_REPS_BYTE_SIZE));
    printf("km_HiKeyMapTypes: %p-%p\n", (void*) pKeymap->km_HiKeyMapTypes, (void*)(((UBYTE*) pKeymap->km_HiKeyMapTypes) + HI_TYPE_LENGTH));
    printf("km_HiKeyMap:      %p-%p\n", (void*) pKeymap->km_HiKeyMap, (void*)(((UBYTE*) pKeymap->km_HiKeyMap) + HI_TYPE_LENGTH * sizeof(ULONG)));
    printf("km_HiCapsable:    %p-%p\n", (void*) pKeymap->km_HiCapsable, (void*)(((UBYTE*) pKeymap->km_HiCapsable) + HI_CAPS_BYTE_SIZE));
    printf("km_HiRepeatable:  %p-%p\n", (void*) pKeymap->km_HiRepeatable, (void*)(((UBYTE*) pKeymap->km_HiRepeatable) + HI_REPS_BYTE_SIZE));
}

const Visitor PrintVisitor = { 
    printNormal,
    printString,
    printDead,
    printNop,
};
