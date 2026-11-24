#include "bt4_ps5.h"
#include "bt4_ps5_memory_layout.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

typedef struct bt4_ps5_region {
    const char *name;
    uintptr_t address;
    size_t size;
    void *mapping;
} bt4_ps5_region_t;

#define BT4_PS5_REGION(name, address, size)                                    \
    {name, (uintptr_t)(address), (size_t)(size), NULL},

static bt4_ps5_region_t regions[] = {
    BT4_PS5_REGION_TABLE(BT4_PS5_REGION)
};

#undef BT4_PS5_REGION

static int fixed_mapping_flags(void) {
#if defined(MAP_FIXED_NOREPLACE)
    return MAP_PRIVATE | MAP_ANON | MAP_FIXED_NOREPLACE;
#elif defined(MAP_EXCL)
    /* FreeBSD/PS5: MAP_EXCL makes MAP_FIXED fail instead of replacing data. */
    return MAP_PRIVATE | MAP_ANON | MAP_FIXED | MAP_EXCL;
#else
#error "A non-destructive fixed mmap flag is required for the PS5 memory probe"
#endif
}

static void release_regions(size_t count) {
    while (count > 0) {
        bt4_ps5_region_t *region = &regions[--count];
        if (region->mapping != NULL) {
            munmap(region->mapping, region->size);
            region->mapping = NULL;
        }
    }
}

uint64_t bt4_ps5_low_address_bytes(void) {
    const size_t count = sizeof(regions) / sizeof(regions[0]);
    uint64_t total = 0;
    size_t index;

    for (index = 0; index < count; ++index) {
        total += (uint64_t)regions[index].size;
    }
    return total;
}

static uint64_t detected_physical_memory(void) {
    const long pages = sysconf(_SC_PHYS_PAGES);
    const long page_size = sysconf(_SC_PAGESIZE);

    if (pages <= 0 || page_size <= 0) {
        return 0;
    }
    return (uint64_t)pages * (uint64_t)page_size;
}

void bt4_ps5_plan_memory(bt4_ps5_memory_budget_t *budget) {
    uint64_t available_for_cache;

    if (budget == NULL) {
        return;
    }

    memset(budget, 0, sizeof(*budget));
    budget->detected_physical_bytes = detected_physical_memory();
    budget->game_budget_bytes = BT4_PS5_GAME_BUDGET_TARGET_BYTES;
    if (budget->detected_physical_bytes != 0 &&
        budget->detected_physical_bytes < budget->game_budget_bytes) {
        budget->game_budget_bytes = budget->detected_physical_bytes;
    }
    budget->low_address_bytes = bt4_ps5_low_address_bytes();

    if (budget->game_budget_bytes <= BT4_PS5_SYSTEM_HEADROOM_BYTES +
                                         BT4_PS5_RUNTIME_RESERVE_BYTES +
                                         budget->low_address_bytes) {
        return;
    }

    available_for_cache = budget->game_budget_bytes -
                          BT4_PS5_SYSTEM_HEADROOM_BYTES -
                          BT4_PS5_RUNTIME_RESERVE_BYTES -
                          budget->low_address_bytes;
    budget->high_cache_bytes = available_for_cache;
    if (budget->high_cache_bytes > BT4_PS5_HIGH_CACHE_LIMIT_BYTES) {
        budget->high_cache_bytes = BT4_PS5_HIGH_CACHE_LIMIT_BYTES;
    }
}

int bt4_ps5_probe_high_memory(size_t bytes, char *error, size_t error_size) {
    const uintptr_t high_boundary = UINT64_C(0x100000000);
    void *mapping;

    if (error != NULL && error_size != 0) {
        error[0] = '\0';
    }
    if (bytes == 0) {
        return 0;
    }

    errno = 0;
    mapping = mmap((void *)high_boundary, bytes, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANON, -1, 0);
    if (mapping == MAP_FAILED) {
        if (error != NULL && error_size != 0) {
            snprintf(error, error_size, "high-memory probe (%lu MiB): %s",
                     (unsigned long)(bytes / (1024 * 1024)), strerror(errno));
        }
        return 1;
    }

    if ((uintptr_t)mapping < high_boundary) {
        if (error != NULL && error_size != 0) {
            snprintf(error, error_size,
                     "high-memory probe returned low address 0x%lx",
                     (unsigned long)(uintptr_t)mapping);
        }
        munmap(mapping, bytes);
        return 2;
    }

    ((volatile uint8_t *)mapping)[0] = 0xC3;
    ((volatile uint8_t *)mapping)[bytes - 1] = 0x3C;
    bt4_ps5_log("high-memory probe mapped %lu MiB at %p",
                (unsigned long)(bytes / (1024 * 1024)), mapping);
    munmap(mapping, bytes);
    return 0;
}

int bt4_ps5_probe_address_space(char *error, size_t error_size) {
    const size_t count = sizeof(regions) / sizeof(regions[0]);
    const int flags = fixed_mapping_flags();
    size_t index;

    if (error != NULL && error_size != 0) {
        error[0] = '\0';
    }

    for (index = 0; index < count; ++index) {
        bt4_ps5_region_t *region = &regions[index];
        void *requested = (void *)region->address;
        void *mapping;

        errno = 0;
        mapping = mmap(requested, region->size, PROT_READ | PROT_WRITE,
                       flags, -1, 0);
        if (mapping == MAP_FAILED || mapping != requested) {
            const int saved_errno = errno;
            if (mapping != MAP_FAILED && mapping != NULL) {
                munmap(mapping, region->size);
            }
            if (error != NULL && error_size != 0) {
                snprintf(error, error_size,
                         "%s at 0x%08lx (%lu KiB): %s",
                         region->name, (unsigned long)region->address,
                         (unsigned long)(region->size / 1024),
                         strerror(saved_errno));
            }
            release_regions(index);
            return (int)index + 1;
        }

        region->mapping = mapping;
        ((volatile uint8_t *)mapping)[0] = 0xA5;
        ((volatile uint8_t *)mapping)[region->size - 1] = 0x5A;
        bt4_ps5_log("mapped %-16s 0x%08lx..0x%08lx",
                    region->name, (unsigned long)region->address,
                    (unsigned long)(region->address + region->size));
    }

    release_regions(count);
    return 0;
}
