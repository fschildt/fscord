#ifndef _POXIS_C_SOURCE
#define _POSIX_C_SOURCE 200809L // enable POSIX.1-2017
#endif

#include <os/os.h>
#include <basic/basic.h>
#include <basic/mem_arena.h>
#include <crypto/rsa.h>

#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    u16 port;
} ParsedArgs; 

internal_fn b32
parse_args(ParsedArgs *args, int argc, char **argv)
{
    u16 port;
    char *key_filepath;

    if (argc != 5) goto format_err;

    if (strcmp(argv[1], "-port") != 0) {
        goto format_err;
    }
    port = atoi(argv[2]);
    if (port == 0) {
        printf("port number is invalid\n");
        return false;
    }

    args->port = port;
    return true;

format_err:
    printf("invocation format error\n");
    return false;
}

int
main(int argc, char **argv)
{
    MemArena arena;
    OSMemory memory;
    if (!os_memory_allocate(&memory, MEBIBYTES(1))) {
        return EXIT_FAILURE;
    }
    mem_arena_init(&arena, memory.p, memory.size);


    debug_printf("parsing args...\n");
    ParsedArgs args;
    if (!parse_args(&args, argc, argv)) {
        return EXIT_FAILURE;
    }


    debug_printf("creating rsa from file...\n");
    EVP_PKEY *rsa_key_pri = rsa_create_via_file(&arena, "./server_prikey.pem", false);

    debug_printf("creating secure stream listener...\n");
    OSNetSecureStream *listener = os_net_secure_stream_listen(args.port, rsa_key_pri);
    if (!listener) {
        return EXIT_FAILURE;
    }

    for (;;) {
        debug_printf("accepting connection...\n");
        OSNetSecureStream *client = os_net_secure_stream_accept(listener);
        if (!client) {
            break;
        }

        printf("accepted a connection...\n");
    }

    return EXIT_FAILURE;
}
