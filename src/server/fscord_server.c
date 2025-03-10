#ifndef _POXIS_C_SOURCE
#define _POSIX_C_SOURCE 200809L // enable POSIX.1-2017
#endif

#include <os/os.h>
#include <basic/basic.h>
#include <basic/mem_arena.h>
#include <basic/string32.h>
#include <server/client_connections.h>

#include <stdio.h>
#include <string.h>


typedef struct {
    u16 port;
} ParsedArgs; 


internal_fn b32
parse_args(ParsedArgs *args, int argc, char **argv)
{
    if (argc != 3) {
        goto format_err;
    }


    if (strcmp(argv[1], "-port") != 0) {
        goto format_err;
    }

    u16 port = atoi(argv[2]);
    if (port == 0) {
        printf("port number is invalid\n");
        return false;
    }


    args->port = port;
    return true;


format_err:
    printf("invocation format error, execpting \"-port <portnum>\"\n");
    return false;
}


int
main(int argc, char **argv)
{
    OSMemory memory;
    if (!os_memory_allocate(&memory, MEBIBYTES(2))) {
        return EXIT_FAILURE;
    }

    MemArena arena;
    mem_arena_init(&arena, memory.p, memory.size);


    ParsedArgs args;
    if (!parse_args(&args, argc, argv)) {
        return EXIT_FAILURE;
    }

    ClientConnections *client_connections = client_connections_create(&arena, args.port);
    if (!client_connections) {
        return EXIT_FAILURE;
    }
    client_connections_manage(client_connections);


    return EXIT_SUCCESS;
}
