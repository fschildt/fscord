#ifndef _POXIS_C_SOURCE
#define _POSIX_C_SOURCE 200809L // enable POSIX.1-2017
#endif

#include <os/os.h>
#include <basic/basic.h>
#include <basic/mem_arena.h>
#include <basic/string32.h>
#include <crypto/rsa.h>
#include <os/os.h>
#include <messages/messages.h>
#include <server/client_connection.h>

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
    MemArena arena;
    OSMemory memory;
    if (!os_memory_allocate(&memory, MEBIBYTES(2))) {
        return EXIT_FAILURE;
    }
    mem_arena_init(&arena, memory.p, memory.size);


    ParsedArgs args;
    if (!parse_args(&args, argc, argv)) {
        return EXIT_FAILURE;
    }


    os_net_secure_streams_init(&arena, MESSAGES_MAX_USER_COUNT + 1);


    EVP_PKEY *server_rsa_pri = rsa_create_via_file(&arena, "./server_rsa_pri.pem", false);

    u32 listener = os_net_secure_stream_listen(args.port, server_rsa_pri);
    if (listener == OS_NET_SECURE_STREAM_ID_INVALID) {
        return EXIT_FAILURE;
    }


    #if 0
    if (!client_connections_create(&arena)) {
        return EXIT_FAILURE;
    }
    #endif


    printf("listening on port %d\n", args.port);
    for (;;) {
        u32 secure_stream_id = os_net_secure_stream_accept(listener);
        if (secure_stream_id != OS_NET_SECURE_STREAM_ID_INVALID) {
            client_connection_add(secure_stream_id);
        }
    }


    return EXIT_SUCCESS;
}
