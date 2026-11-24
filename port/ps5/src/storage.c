#include "bt4_ps5.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <unistd.h>

#if defined(__FreeBSD__)
#include <sys/mount.h>
#endif

typedef struct bt4_storage_candidate {
    const char *content_root;
    const char *mount_root;
    int is_external;
} bt4_storage_candidate_t;

#define USB_CANDIDATES(number)                                                \
    {"/mnt/usb" #number "/bt4", "/mnt/usb" #number, 1},                   \
    {"/mnt/usb" #number "/homebrew/bt4", "/mnt/usb" #number, 1}

#define EXT_CANDIDATES(number)                                                \
    {"/mnt/ext" #number "/bt4", "/mnt/ext" #number, 1},                   \
    {"/mnt/ext" #number "/homebrew/bt4", "/mnt/ext" #number, 1}

static const bt4_storage_candidate_t candidates[] = {
    USB_CANDIDATES(0),
    USB_CANDIDATES(1),
    USB_CANDIDATES(2),
    USB_CANDIDATES(3),
    USB_CANDIDATES(4),
    USB_CANDIDATES(5),
    USB_CANDIDATES(6),
    USB_CANDIDATES(7),
    EXT_CANDIDATES(0),
    EXT_CANDIDATES(1),
    {BT4_PS5_DATA_ROOT, BT4_PS5_DATA_ROOT, 0},
};

#undef USB_CANDIDATES
#undef EXT_CANDIDATES

static int directory_exists(const char *path) {
    struct stat status;

    return path != NULL && stat(path, &status) == 0 && S_ISDIR(status.st_mode);
}

static void detect_filesystem(const char *mount_root,
                              char *filesystem,
                              size_t filesystem_size) {
#if defined(__FreeBSD__)
    struct statfs info;

    if (statfs(mount_root, &info) == 0) {
        snprintf(filesystem, filesystem_size, "%s", info.f_fstypename);
        return;
    }
#else
    (void)mount_root;
#endif
    snprintf(filesystem, filesystem_size, "unknown");
}

static int describe_candidate(const bt4_storage_candidate_t *candidate,
                              bt4_ps5_storage_t *storage) {
    struct statvfs volume;

    if (!directory_exists(candidate->content_root) ||
        statvfs(candidate->content_root, &volume) != 0) {
        return 0;
    }

    memset(storage, 0, sizeof(*storage));
    snprintf(storage->content_root, sizeof(storage->content_root), "%s",
             candidate->content_root);
    snprintf(storage->mount_root, sizeof(storage->mount_root), "%s",
             candidate->mount_root);
    detect_filesystem(candidate->mount_root, storage->filesystem,
                      sizeof(storage->filesystem));
    storage->free_bytes = (uint64_t)volume.f_bavail *
                          (uint64_t)volume.f_frsize;
    storage->is_external = candidate->is_external;
    storage->is_writable = access(candidate->content_root, W_OK) == 0;
    return 1;
}

int bt4_ps5_find_content_root(const char *preferred,
                              bt4_ps5_storage_t *storage,
                              char *error,
                              size_t error_size) {
    size_t index;

    if (error != NULL && error_size != 0) {
        error[0] = '\0';
    }
    if (storage == NULL) {
        if (error != NULL && error_size != 0) {
            snprintf(error, error_size, "storage result is null");
        }
        return 1;
    }

    if (preferred != NULL && preferred[0] != '\0') {
        const bt4_storage_candidate_t explicit_candidate = {
            preferred, preferred, strncmp(preferred, "/data/", 6) != 0};
        if (describe_candidate(&explicit_candidate, storage)) {
            return 0;
        }
        if (error != NULL && error_size != 0) {
            snprintf(error, error_size,
                     "content root '%s' is not an accessible directory",
                     preferred);
        }
        return 2;
    }

    for (index = 0; index < sizeof(candidates) / sizeof(candidates[0]); ++index) {
        if (describe_candidate(&candidates[index], storage)) {
            return 0;
        }
    }

    if (error != NULL && error_size != 0) {
        snprintf(error, error_size,
                 "no BT4 directory found on USB, M.2, or %s",
                 BT4_PS5_DATA_ROOT);
    }
    return 3;
}
