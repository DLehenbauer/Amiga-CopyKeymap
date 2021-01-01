#include "../visit.h"
#include <proto/keymap.h>

typedef struct {
    ULONG* pKmEntry;
    UBYTE* pBuffer;
    UBYTE deadCharTableBytes;
} CopyContext;

extern const Visitor CopyVisitor;
