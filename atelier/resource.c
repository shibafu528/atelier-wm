#include <string.h>
#include <unistd.h>
#include <libgen.h>

static void GetInstalledDirectory(char* buffer, int buffer_length) {
    char path[1024] = {};
    char base[1024];
    readlink("proc/self/exe", path, sizeof(path) - 1);
    base = basename(path);
    strncpy(buffer, base, buffer_length);
}

