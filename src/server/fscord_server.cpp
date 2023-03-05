#ifndef _POXIS_C_SOURCE
#define _POSIX_C_SOURCE 200809L // enable POSIX.1-2017
#endif

#include <common/fscord_defs.h>
#include <common/fscord_net.h>

struct ParsedArgs {
    uint16_t port;
    const char *key_filepath;
    const char *password;
};

#include <Rsa.h>
#include <ClientManager.h>

#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>

internal int
open_listening_socket(u16 port, int backlog)
{
    int listening_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (listening_socket == -1) {
        perror("socket()");
        return -1;
    }

    int enable = 1;
    if (setsockopt(listening_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        perror("setsockopt(SO_REUSEADDR)");
        close(listening_socket);
        return -1;
    }

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);

    if (bind(listening_socket, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("bind()");
        close(listening_socket);
        return -1;
    }

    if (listen(listening_socket, backlog) == -1) {
        perror("listen()");
        close(listening_socket);
        return -1;
    }

    return listening_socket;
}

internal bool
parse_args(ParsedArgs *args, int argc, char **argv) {
    u16 port;
    const char *key_filepath;


    if (argc != 5) goto format_err;

    if (strcmp(argv[1], "-port") != 0) {
        goto format_err;
    }
    port = atoi(argv[2]);
    if (port == 0) {
        printf("port number is invalid\n");
        return false;
    }

    if (strcmp(argv[3], "-keyfile") != 0) {
        goto format_err;
    }
    key_filepath = argv[4];

    args->port = port;
    args->key_filepath = key_filepath;
    return true;

format_err:
    printf("invocation format error:\n");
    printf("received: ");
    for (int i = 0; i < argc; i++) {
        printf("%s ", argv[i]);
    }
    printf("\n");
    printf("expected: path/to/fscord_server -port <num> -keyfile <path> -password <password>\n");
    return false;
}

int main(int argc, char **argv) {
    s32 backlog = 32;

    ParsedArgs parsed_args;
    if (!parse_args(&parsed_args, argc, argv)) {
        return EXIT_FAILURE;
    }

    int listening_fd = open_listening_socket(parsed_args.port, backlog);
    if (listening_fd == -1) {
        return EXIT_FAILURE;
    }

    Rsa rsa;
    if (!rsa.Init(parsed_args.key_filepath)) {
        return EXIT_FAILURE;
    }

    ClientManager client_manager;
    if (!client_manager.Init(&rsa, listening_fd)) {
        return EXIT_FAILURE;
    }

    client_manager.Run();
    return EXIT_FAILURE;
}
