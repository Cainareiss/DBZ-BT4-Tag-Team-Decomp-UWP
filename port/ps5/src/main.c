#include "bt4_ps5.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static const char *requested_content_root(int argc, char **argv) {
    const char prefix[] = "--content-root=";
    int index;

    for (index = 1; index < argc; ++index) {
        if (strncmp(argv[index], prefix, sizeof(prefix) - 1) == 0) {
            return argv[index] + sizeof(prefix) - 1;
        }
    }
    return getenv("BT4_CONTENT_ROOT");
}

int main(int argc, char **argv) {
    char error[256];
    bt4_ps5_memory_budget_t budget;
    bt4_ps5_storage_t storage;
    const char *content_root;
    int result;

    bt4_ps5_log("milestone 0 bootstrap started");
    bt4_ps5_notify("BT4 PS5: testing low/high memory and storage...");

    bt4_ps5_plan_memory(&budget);
    bt4_ps5_log("memory plan: physical %.2f GiB, game %.2f GiB, "
                "low %.1f MiB, high cache up to %.2f GiB",
                (double)budget.detected_physical_bytes / (1024.0 * 1024.0 * 1024.0),
                (double)budget.game_budget_bytes / (1024.0 * 1024.0 * 1024.0),
                (double)budget.low_address_bytes / (1024.0 * 1024.0),
                (double)budget.high_cache_bytes / (1024.0 * 1024.0 * 1024.0));

    content_root = requested_content_root(argc, argv);
    result = bt4_ps5_find_content_root(content_root, &storage, error,
                                       sizeof(error));
    if (result != 0) {
        bt4_ps5_log("storage discovery failed: %s", error);
    } else {
        bt4_ps5_log("content root: %s (%s, %s, %.2f GiB free)",
                    storage.content_root, storage.filesystem,
                    storage.is_external ? "external" : "internal",
                    (double)storage.free_bytes / (1024.0 * 1024.0 * 1024.0));
    }

    result = bt4_ps5_probe_address_space(error, sizeof(error));
    if (result != 0) {
        char message[320];
        snprintf(message, sizeof(message),
                 "BT4 PS5 port blocked: %s. See %s", error,
                 BT4_PS5_LOG_PATH);
        bt4_ps5_log("memory probe failed: %s", error);
        bt4_ps5_notify(message);
        return result;
    }

    result = bt4_ps5_probe_high_memory(BT4_PS5_HIGH_MEMORY_PROBE_BYTES,
                                       error, sizeof(error));
    if (result != 0) {
        char message[320];
        snprintf(message, sizeof(message),
                 "BT4 PS5 high-memory test failed: %s", error);
        bt4_ps5_log("high-memory probe failed: %s", error);
        bt4_ps5_notify(message);
        return 20 + result;
    }

    bt4_ps5_log("memory probes passed; fixed-address core plus high cache is viable");
    bt4_ps5_notify("BT4 PS5: memory/storage test passed. Core bootstrap is next.");
    return 0;
}
