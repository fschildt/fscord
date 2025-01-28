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
    u16 port;

    if (argc != 5) {
        goto format_err;
    }

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


    printf("parsing args...\n");
    ParsedArgs args;
    if (!parse_args(&args, argc, argv)) {
        return EXIT_FAILURE;
    }


    printf("create rsa via file...\n");
    EVP_PKEY *server_rsa_pri = rsa_create_via_file(&arena, "./server_rsa_pri.pem", false);

    printf("create secure stream listener...\n");
    OSNetSecureStream *listener = os_net_secure_stream_listen(args.port, server_rsa_pri);
    if (!listener) {
        return EXIT_FAILURE;
    }

    printf("create client connections...\n");
    if (!client_connections_create(&arena)) {
        return EXIT_FAILURE;
    }


    printf("looping for accept...\n");
    for (;;) {
        debug_printf("accepting connection...\n");
        OSNetSecureStream *client_stream = os_net_secure_stream_accept(listener);
        if (!client_stream) {
            printf("error: not accepted a new client secure stream\n");
            continue;
        }

        i16 client_connection = client_connection_alloc_and_init(client_stream);
        if (client_connection < 0) {
            os_net_secure_stream_close(client_stream);
            continue;
        }



        printf("accepted a connection...\n");
    }

    return EXIT_FAILURE;
}
