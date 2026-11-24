#include "bt4_ps5.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void bt4_ps5_log(const char *format, ...) {
    va_list args;

    va_start(args, format);
    vfprintf(stderr, format, args);
    fputc('\n', stderr);
    va_end(args);
}

int main(void) {
    bt4_ps5_memory_budget_t budget;
    bt4_ps5_storage_t storage;
    char error[256];

    bt4_ps5_plan_memory(&budget);
    if (budget.low_address_bytes == 0 || budget.game_budget_bytes == 0 ||
        budget.high_cache_bytes == 0) {
        fprintf(stderr, "invalid memory plan\n");
        return EXIT_FAILURE;
    }

    if (bt4_ps5_probe_high_memory(16U * 1024U * 1024U, error,
                                   sizeof(error)) != 0) {
        fprintf(stderr, "%s\n", error);
        return EXIT_FAILURE;
    }

    if (bt4_ps5_find_content_root(".", &storage, error, sizeof(error)) != 0) {
        fprintf(stderr, "%s\n", error);
        return EXIT_FAILURE;
    }

    printf("host smoke: %.1f MiB low, %.2f GiB cache, storage %s\n",
           (double)budget.low_address_bytes / (1024.0 * 1024.0),
           (double)budget.high_cache_bytes / (1024.0 * 1024.0 * 1024.0),
           storage.content_root);
    return EXIT_SUCCESS;
}
