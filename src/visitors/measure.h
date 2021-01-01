#include "../visit.h"

typedef struct {
    int stringEntries;
    int stringBytes;
    int deadEntries;
    int modEntries;
    int deadCharTableBytes;
} Sizes;

extern const Visitor MeasureVisitor;
