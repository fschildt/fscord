#include <dlfcn.h>
#include <stdio.h>
#include <SDL2/SDL.h>

typedef void PlatformLibrary;

PlatformLibrary *platform_library_open(const char *path) {
    void *handle = dlopen(path, RTLD_NOW);
    if (!handle) {
        printf("could not open library %s\n", path);
        return 0;
    }

    return handle;
}

void platform_library_close(PlatformLibrary *handle) {
    dlclose(handle);
}

void *platform_library_get_proc(PlatformLibrary *handle, const char *name) {
    void *addr = dlsym(handle, name);
    return addr;
}

