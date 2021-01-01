#include <proto/keymap.h>

#ifndef KEYMAPTABLE_H
#define KEYMAPTABLE_H

#define LO_TYPE_LENGTH      64  /* UBYTE *km_LoKeyMapTypes                      */
#define LO_MAP_LENGTH       64  /* ULONG *km_LoKeyMap;                          */
#define LO_CAPS_BYTE_SIZE    8  /* UBYTE *km_LoCapsable     (1 bit per entry)   */
#define LO_REPS_BYTE_SIZE    8  /* UBYTE *km_LoRepeatable;  (1 bit per entry)   */
#define HI_TYPE_LENGTH      56  /* UBYTE *km_HiKeyMapTypes;                     */
#define HI_MAP_LENGTH       56  /* ULONG *km_HiKeyMap;                          */
#define HI_CAPS_BYTE_SIZE    7  /* UBYTE *km_HiCapsable;    (1 bit per entry)   */
#define HI_REPS_BYTE_SIZE    7  /* UBYTE *km_HiRepeatable;  (1 bit per entry)   */

typedef struct {
    ULONG loKeyMap[LO_MAP_LENGTH];
    ULONG hiKeyMap[HI_MAP_LENGTH];
    UBYTE loKeyMapTypes[LO_TYPE_LENGTH];
    UBYTE hiKeyMapTypes[HI_TYPE_LENGTH];
    UBYTE loCapsable[LO_CAPS_BYTE_SIZE];
    UBYTE hiCapsable[HI_CAPS_BYTE_SIZE];
    UBYTE loRepeatable[LO_REPS_BYTE_SIZE];
    UBYTE hiRepeatable[HI_REPS_BYTE_SIZE];
} KeyMapTables;

#endif