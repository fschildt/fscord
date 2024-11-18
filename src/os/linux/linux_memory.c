#include <os/os.h>
#include <basic/basic.h>

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>

b32
os_memory_allocate(OSMemory *memory, size_t size)
{
    // @Performance: Keep the fd of /dev/zero open for the whole runtime?
    int fd = open("/dev/zero", O_RDWR);
    if (fd == -1) {
        printf("open() failed: %s\n", strerror(errno));
        return false;
    }

    void *address = mmap(0, size, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (address == MAP_FAILED) {
        printf("mmap failed\n");
        return false;
    }

    close(fd);

    memory->size = size;
    memory->p = address;
    return true;
}

void
os_memory_free(OSMemory *memory)
{
    munmap(memory->p, memory->size);
}

// Todo: move to other file.
OSTime
os_time_get_now()
{
    struct timespec ts = {};
    i32 err = clock_gettime(CLOCK_REALTIME, &ts);
    if (err != 0) {
        printf("failed to get time\n");
    }

    OSTime result = {};
    result.seconds = ts.tv_sec;
    result.nanoseconds = ts.tv_nsec;
    return result;
}

// Todo: move to other file
char *
os_read_file_as_string(MemArena *arena, char *filepath, size_t *len)
{
    int fd = open(filepath, O_RDONLY);
    if (fd == -1) {
        printf("can't open file %s for reading\n", filepath);
        return 0;
    }

    struct stat statbuf;
    if (fstat(fd, &statbuf) == -1) {
        printf("can't fstat file %s\n", filepath);
        return 0;
    }

    char *dest = mem_arena_push(arena, statbuf.st_size+1);
    ssize_t bytes_read = read(fd, dest, statbuf.st_size);
    if (bytes_read != statbuf.st_size) {
        printf("only read %ld/%ld bytes\n", statbuf.st_size, bytes_read);
        arena->size_used -= statbuf.st_size+1; // Todo: cleanup
        return 0;
    }
    dest[bytes_read] = '\0';
    *len = statbuf.st_size;

    return dest;
}


