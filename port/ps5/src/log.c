#include "bt4_ps5.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

typedef struct bt4_notify_request {
    char reserved[45];
    char message[3075];
} bt4_notify_request_t;

int sceKernelSendNotificationRequest(int, bt4_notify_request_t *, size_t, int);

static void ensure_log_directory(void) {
    if (mkdir(BT4_PS5_DATA_ROOT, 0777) != 0 && errno != EEXIST) {
        return;
    }
    (void)mkdir(BT4_PS5_LOG_DIR, 0777);
}

void bt4_ps5_log(const char *format, ...) {
    char line[1024];
    va_list args;
    FILE *file;

    va_start(args, format);
    vsnprintf(line, sizeof(line), format, args);
    va_end(args);

    fprintf(stderr, "BT4-PS5: %s\n", line);
    fflush(stderr);

    ensure_log_directory();
    file = fopen(BT4_PS5_LOG_PATH, "a");
    if (file != NULL) {
        fprintf(file, "%s\n", line);
        fclose(file);
    }
}

void bt4_ps5_notify(const char *message) {
    bt4_notify_request_t request;

    memset(&request, 0, sizeof(request));
    snprintf(request.message, sizeof(request.message), "%s", message);
    (void)sceKernelSendNotificationRequest(0, &request, sizeof(request), 0);
}
