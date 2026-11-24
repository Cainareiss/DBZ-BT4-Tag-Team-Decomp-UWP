#ifndef BT4_PS5_MEMORY_LAYOUT_H
#define BT4_PS5_MEMORY_LAYOUT_H

/*
 * Keep this table in ascending address order.  It mirrors the native x86-64
 * layout in port/src/plat_mem.c and adds a reservation for the secondary game
 * image.  The PS5 bootstrap itself remains position independent.
 */
#define BT4_PS5_REGION_TABLE(X)                                                \
    X("game heap",       0x003BE000u, 0x01C42000u)                            \
    X("game stack",      0x02000000u, 0x01000000u)                            \
    X("hardware regs",   0x10000000u, 0x00010000u)                            \
    X("GS regs",         0x12000000u, 0x00002000u)                            \
    X("host low arena",  0x13000000u, 0x0C000000u)                            \
    X("game image",      0x20000000u, 0x04000000u)                            \
    X("scratchpad",      0x70000000u, 0x00004000u)

#define BT4_PS5_GAME_IMAGE_BASE 0x20000000u

#endif
