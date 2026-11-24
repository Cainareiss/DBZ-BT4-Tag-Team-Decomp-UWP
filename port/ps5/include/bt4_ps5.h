#ifndef BT4_PS5_H
#define BT4_PS5_H

#include <stddef.h>
#include <stdint.h>

#define BT4_PS5_DATA_ROOT "/data/bt4"
#define BT4_PS5_LOG_DIR BT4_PS5_DATA_ROOT "/logs"
#define BT4_PS5_LOG_PATH BT4_PS5_LOG_DIR "/ps5-port.log"
#define BT4_PS5_PATH_CAPACITY 1024

/*
 * Retail PS5 game processes have roughly 12 GiB available from the unified
 * memory pool.  This is a planning ceiling, not an eager allocation request:
 * the actual process limit is checked at runtime and memory is committed only
 * when a subsystem needs it.
 */
#define BT4_PS5_GAME_BUDGET_TARGET_BYTES (12ULL * 1024ULL * 1024ULL * 1024ULL)
#define BT4_PS5_SYSTEM_HEADROOM_BYTES (2ULL * 1024ULL * 1024ULL * 1024ULL)
#define BT4_PS5_RUNTIME_RESERVE_BYTES (1ULL * 1024ULL * 1024ULL * 1024ULL)
#define BT4_PS5_HIGH_CACHE_LIMIT_BYTES (8ULL * 1024ULL * 1024ULL * 1024ULL)
#define BT4_PS5_HIGH_MEMORY_PROBE_BYTES (64ULL * 1024ULL * 1024ULL)

typedef struct bt4_ps5_memory_budget {
    uint64_t detected_physical_bytes;
    uint64_t game_budget_bytes;
    uint64_t low_address_bytes;
    uint64_t high_cache_bytes;
} bt4_ps5_memory_budget_t;

typedef struct bt4_ps5_storage {
    char content_root[BT4_PS5_PATH_CAPACITY];
    char mount_root[BT4_PS5_PATH_CAPACITY];
    char filesystem[32];
    uint64_t free_bytes;
    int is_external;
    int is_writable;
} bt4_ps5_storage_t;

void bt4_ps5_log(const char *format, ...);
void bt4_ps5_notify(const char *message);

/*
 * Safely reserves every low-address region needed by the current x86-64
 * decomp build, writes to its boundary pages, then releases it.  No existing
 * mapping is replaced.  Returns zero on success and a positive value on the
 * first failed region.
 */
int bt4_ps5_probe_address_space(char *error, size_t error_size);

uint64_t bt4_ps5_low_address_bytes(void);
void bt4_ps5_plan_memory(bt4_ps5_memory_budget_t *budget);
int bt4_ps5_probe_high_memory(size_t bytes, char *error, size_t error_size);

/*
 * Finds BT4 data on /mnt/usb0..7 or /mnt/ext0..1 before falling back to
 * /data/bt4.  An explicit directory can be supplied by --content-root or the
 * BT4_CONTENT_ROOT environment variable.  Saves and logs always stay in
 * /data/bt4 so unplugging a USB drive cannot corrupt them.
 */
int bt4_ps5_find_content_root(const char *preferred,
                               bt4_ps5_storage_t *storage,
                               char *error,
                               size_t error_size);

#endif
