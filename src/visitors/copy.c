#include "copy.h"
#include <exec/types.h>
#include <proto/keymap.h>
#include <assert.h>
#include <string.h>

// 'Normal' kmEntries are copied as-is
void copyNormal(void* pContext, UBYTE rawKey, UBYTE kmType, ULONG kmEntry) {
    CopyContext* pClone = pContext;
    *(pClone->pKmEntry++) = kmEntry;
}

// 'KCF_NOP' kmEntries are copied as-is.
void copyNop(void* pContext, UBYTE rawKey, UBYTE kmType, ULONG kmEntry) {
    CopyContext* pClone = pContext;
    *(pClone->pKmEntry++) = kmEntry;
}

// 'KCF_STRING' entries append a copy of the referenced string table to 'pBuffer'
// and write the address to the kmEntry.
void copyString(void* pContext, UBYTE rawKey, int numEntries, const UBYTE* pSrcTable) {
    CopyContext* pClone = pContext;

    // Remember the start of the src/dest string tables.  We'll need these for
    // computing the string offsets.
    const UBYTE* pSrcStart = pSrcTable;
    UBYTE* pDestTable = pClone->pBuffer;
    const UBYTE* pDestStart = pDestTable;

    // Write the address of the string table to the kmEntry.
    *(pClone->pKmEntry++) = (ULONG) pDestStart;

    // Bump 'pBuffer' to point to the beginning of the char data.  Entering the
    // loop, our pointers are arranged as follows:
    //
    //    pSrcStart,
    //    pSrcTable          pBuffer
    //        |                 |
    //        v                 v
    //        +-----------------+------------------------------+
    //        |     len/off     |           char data          |
    //        | numEntries * 2B |           (Σ lens B)         |
    //        +-----------------+------------------------------+
    //
    pClone->pBuffer += (numEntries << 1);

    for (int n = 0; n < numEntries; n++) {
        const UBYTE len = *(pDestTable++) = *(pSrcTable++);             // Copy length
        UBYTE off = *(pDestTable++) = pClone->pBuffer - pDestStart;     // Compute offset
        memcpy(pClone->pBuffer, pSrcStart + *(pSrcTable++), len);       // Copy bytes
        pClone->pBuffer += len;
    }
}

// 'KCF_DEAD' entries append a copy of the referenced dead table to 'pBuffer'
// and write the address to the kmEntry.
void copyDead(void* pContext, UBYTE rawKey, int numEntries, const UBYTE* pSrcTable) {
    CopyContext* pClone = pContext;

    // Remember the start of the src/dest.  We'll need these for computing the
    // offsets to dead tables for DPF_MOD entries.
    const UBYTE* pSrcStart = pSrcTable;
    UBYTE* pDestTable = pClone->pBuffer;
    const UBYTE* pDestStart = pDestTable;

    // Write the address of the dead table to the kmEntry.
    *(pClone->pKmEntry++) = (ULONG) pDestStart;

    // Bump 'pBuffer' to point to the beginning of the char data.  Entering the
    // loop, our pointers are arranged as follows:
    //
    //    pSrcStart,
    //    pSrcTable          pBuffer
    //        |                 |
    //        v                 v
    //        +-----------------+------------------------------+
    //        |    kind/value   |           dead tables        |
    //        | numEntries * 2B |                              |
    //        +-----------------+------------------------------+
    //
    pClone->pBuffer += (numEntries << 1);

    for (int n = 0; n < numEntries; n++) {
        const UBYTE kind = *(pDestTable++) = *(pSrcTable++);    // copy kind

        switch (kind) {
            case 0:
                *(pDestTable++) = *(pSrcTable++);               // copy char
                break;
            case DPF_DEAD:
                *(pDestTable++) = *(pSrcTable++);               // copy index
                break;
            default:
                assert(kind == DPF_MOD);
                UBYTE off = *(pDestTable++) = pClone->pBuffer - pDestStart;     // Compute offset
                const UBYTE len = pClone->deadCharTableBytes;
                memcpy(pClone->pBuffer, pSrcStart + *(pSrcTable++), len);       // Copy table
                pClone->pBuffer += len;
                break;
        }
    }
}

const Visitor CopyVisitor = { 
    copyNormal,
    copyString,
    copyDead,
    copyNop,
};
