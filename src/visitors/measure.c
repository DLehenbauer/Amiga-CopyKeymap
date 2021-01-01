#include <exec/types.h>
#include <assert.h>
#include "measure.h"

void measureNop(void* pContext, UBYTE rawKey, UBYTE kmType, ULONG kmEntry) { }

void measureString(void* pContext, UBYTE rawKey, int numEntries, const UBYTE* pKmEntry) {
    Sizes* pSizes = pContext;

    // Count the number of KCF_STRING entries.  Each will need 2B (len/offset) reserved
    // for the string table.
    pSizes->stringEntries += numEntries;

    // Sum the string lengths for all entries for this key.
    for (int n = 0; n < numEntries; n++) {
        pSizes->stringBytes += *pKmEntry;   // Add the entry's string length
        pKmEntry += 2;                      // Skip to the next string length
    }
}

void measureDead(void* pContext, UBYTE rawKey, int numEntries, const UBYTE* pKmEntry) {
    Sizes* pSizes = pContext;

    // Count the number of KCF_DEAD entries.  Each will need 2B (kind/value) reserved
    // for the string table.
    pSizes->deadEntries += numEntries;

    // Scan DPF_DEAD entries to calculate the maximum dead key index in the table.
    // (The max dead key index determines the length of the dead tables.)
    //
    // Scan DPF_MOD entries to calculate the number of dead tables required.
    for (int n = 0; n < numEntries; n++) {
        int deadIndex;

        UBYTE kind = *(pKmEntry++);
        UBYTE value = *(pKmEntry++);

        switch (kind) {
            case 0:
                break;
            case DPF_DEAD: {
                // If a double-dead key, the high nibble contains the multiplier and the low
                // nibble the maximum index.  Otherwise contains the max index.
                const UBYTE maxBytes = (value >> DP_2DFACSHIFT)
                    ? ((value >> DP_2DFACSHIFT) * (value & DP_2DINDEXMASK))
                    : (value + 1);      // +1 to account for the key when unprefixed by a dead key.

                if (maxBytes > pSizes->deadCharTableBytes) {
                    pSizes->deadCharTableBytes = maxBytes;
                }
                break;
            }
            default:
                // Each DPF_MOD key will require a dead table of 'maxBytes' (computed above).
                assert(kind == DPF_MOD);
                pSizes->modEntries++;
                break;
        }
    }
}

const Visitor MeasureVisitor = {
    measureNop,
    measureString,
    measureDead,
    measureNop,
};
