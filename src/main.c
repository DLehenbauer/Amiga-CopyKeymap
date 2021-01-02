#include "visit.h"
#include "visitors/measure.h"
#include "visitors/copy.h"
#include "visitors/print.h"
#include <proto/exec.h>
#include <proto/keymap.h>
#include <devices/conunit.h>
#include <assert.h>
#include <string.h>
#include "keymaptable.h"

#define ALIGN16(x) ((((ULONG) x) + 1) & ~1)

struct Library* KeymapBase;

struct KeyMap* readKeymap() {
    struct KeyMap* pKeymap;

    if (KeymapBase = OpenLibrary("keymap.library", 37L)) {
        pKeymap = AskKeyMapDefault();
        CloseLibrary(KeymapBase);

        return pKeymap;
    }

    return NULL;
}

void setKeymap(struct KeyMap* pKeymap) {
    if (KeymapBase = OpenLibrary("keymap.library", 37L)) {
        SetKeyMapDefault(pKeymap);
        CloseLibrary(KeymapBase);
    }
}

struct KeyMap* copyKeymap(struct KeyMap* pSrc) {
    // To calculate the amount of memory required to hold a copy of the keymap,
    // we use a visitor to walk the keymap and count the following quantities:
    //
    //  - The number of KCF_STRING entries
    //  - The total number of characters referenced by the KCF_STRING entries
    //  - The number of KCF_DEAD entries
    //  - The number of KCF_DEAD entries that are DPF_MOD
    //  - The size of the dead char table for each DPF_MOD entry
    //
    // (See the measure visitor for more details.)
    Sizes sizes = { 0 };
    visit(pSrc, /* pContext: */ &sizes, /* visitor: */ MeasureVisitor);

    // There are three pieces to the KeyMap we need to copy:
    //
    //  - The keymap struct itself (8 pointers = 32B)
    //  - The fixed size tables pointed to by the keymap struct containing the maps, types, capsable
    //    and repeatable arrays for the lo and hi maps (630B)
    //  - The string and dead char entries pointed to by KCF_STRING and KCF_DEAD entries.
    //    (size calculated below)
    //
    const ULONG keymapSize = ALIGN16(sizeof(struct KeyMap));    // 8 pointers = 64B
    const ULONG tablesSize = ALIGN16(sizeof(KeyMapTables));     // 8 arrays, totaling 630B
    const ULONG bufferSize =(sizes.stringEntries << 1)  // Each KCF_STRING = 2 bytes (UBYTE length, UBYTE offset)
        + sizes.stringBytes                             // Sum of KCF_STRING lengths
        + (sizes.deadEntries << 1)                      // Each KCF_DEAD = 2 bytes (UBYTE kind, UBYTE char/index/offset)
        + sizes.deadCharTableBytes * sizes.modEntries;  // Each kind=DPF_MOD points to a dead char table

    const ULONG size =
        keymapSize          // 32B
        + tablesSize        // 630B
        + bufferSize;       // variable length

    // Allocate a contiguous block of memory to hold the copy of the keymap and compute
    // pointers into the memory for the various table as follows:
    //
    //      pDestKeyMap -> +-----------------------+
    //                     |           64B         |
    //      pDestTables -> +-----------------------+
    //                     |                       |
    //                     |          630B         |
    //                     |                       |
    //      pDestBuffer -> +-----------------------+
    //                     |                       |
    //                     | String & Dead Tables  |
    //                     |                       |
    //                     +-----------------------+
    //
    struct KeyMap* pDestKeyMap = AllocMem(size, MEMF_CLEAR | MEMF_PUBLIC);
    KeyMapTables* pDestTables = (KeyMapTables*)(((UBYTE*) pDestKeyMap) + keymapSize);
    UBYTE* pDestBuffer = ((UBYTE*) pDestTables) + tablesSize;
    const UBYTE* pDestBufferStart = pDestBuffer;

    // Initialize the KeyMap struct with the addresses of the fixed-size map, type,
    // capsable, and repeatable tables.
    pDestKeyMap->km_LoKeyMapTypes   = pDestTables->loKeyMapTypes;
    pDestKeyMap->km_LoKeyMap        = pDestTables->loKeyMap;
    pDestKeyMap->km_LoCapsable      = pDestTables->loCapsable;
    pDestKeyMap->km_LoRepeatable    = pDestTables->loRepeatable;
    pDestKeyMap->km_HiKeyMapTypes   = pDestTables->hiKeyMapTypes;
    pDestKeyMap->km_HiKeyMap        = pDestTables->hiKeyMap;
    pDestKeyMap->km_HiCapsable      = pDestTables->hiCapsable;
    pDestKeyMap->km_HiRepeatable    = pDestTables->hiRepeatable;

    // The hi/lo type, capsable, and repeatable tables do not contain pointers and
    // can simply be memcpy'ed.
    memcpy(pDestKeyMap->km_LoKeyMapTypes, pSrc->km_LoKeyMapTypes, LO_TYPE_LENGTH);
    memcpy(pDestKeyMap->km_LoCapsable,    pSrc->km_LoCapsable,    LO_CAPS_BYTE_SIZE);
    memcpy(pDestKeyMap->km_LoRepeatable,  pSrc->km_LoRepeatable,  LO_REPS_BYTE_SIZE);
    memcpy(pDestKeyMap->km_HiKeyMapTypes, pSrc->km_HiKeyMapTypes, HI_TYPE_LENGTH);
    memcpy(pDestKeyMap->km_HiCapsable,    pSrc->km_HiCapsable,    HI_CAPS_BYTE_SIZE);
    memcpy(pDestKeyMap->km_HiRepeatable,  pSrc->km_HiRepeatable,  HI_REPS_BYTE_SIZE);

    // The hi/lo map entries contain pointers to string/dead tables.  To copy these,
    // we use a visitor that walks the map entries, inspects the associated type,
    // and appends copies of referenced string/dead tables to the preallocated buffer
    // that begins at 'pDestBuffer'.  See the copy visitor for more details.
    CopyContext copy = { 0 };
    copy.pKmEntry = pDestTables->loKeyMap;                  // Next KmEntry to write
    copy.pBuffer = pDestBuffer;                             // Next available space for string/dead table
    copy.deadCharTableBytes = sizes.deadCharTableBytes;     // Size of dead table (per DPF_MOD entry)

    // Copy the map entries and string/dead tables for the low map.
    visitLo(pSrc, &copy, CopyVisitor);
    
    // Sanity check that CopyVisitor advanced pKmEntry to the end of the low map.
    assert((copy.pKmEntry - pDestKeyMap->km_LoKeyMap) == LO_MAP_LENGTH);

    // Copy the map entries and string/dead tables for the high map.
    copy.pKmEntry = pDestTables->hiKeyMap;                  // Update 'pKmEntry' to start of high map.
    visitHi(pSrc, &copy, CopyVisitor);

    // Sanity check that CopyVisitor advanced pKmEntry to the end of the high map.
    assert(copy.pKmEntry - pDestKeyMap->km_HiKeyMap == HI_MAP_LENGTH);

    // Sanity check that the total number of bytes used by the CopyVisitor for
    // string/dead tables matches the sum calculated by the MeasureVisitor.
    assert(copy.pBuffer == pDestBufferStart + bufferSize);

    return pDestKeyMap;
}

int main() {
    struct KeyMap* pSrc = readKeymap();

    Sizes sizes = { 0 };
    visit(pSrc, /* pContext: */ &sizes, /* visitor: */ MeasureVisitor);
    visit(pSrc, /* pContext: */ &sizes, /* visitor: */ PrintVisitor);

    struct KeyMap* pCopy = copyKeymap(pSrc);
    visit(pCopy, /* pContext: */ &sizes, /* visitor: */ PrintVisitor);

    setKeymap(pCopy);

    return 0;
}
