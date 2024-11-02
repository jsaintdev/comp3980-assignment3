//
// Created by justinsaint on 01/11/24.
//

include "client.h"

int main(int argc, char *argv[])
{
    char                   *address;
    char                   *port_str;
    in_port_t               port;
    int                     sockfd;
    struct sockaddr_storage addr;

    address  = NULL;
    port_str = NULL;
    parse_arguments(argc, argv, &address, &port_str);
    handle_arguments(argv[0], address, port_str, &port);
    convert_address(address, &addr);
    sockfd = socket_create(addr.ss_family, SOCK_STREAM, 0);
    socket_connect(sockfd, &addr, port);
    socket_close(sockfd);

    return EXIT_SUCCESS;
}

_Noreturn static void usage(const char *program_name, int exit_code, const char *message)
{
    if(message)
    {
        fprintf(stderr, "%s\n", message);
    }

    fprintf(stderr, "Usage: %s [-h] -s <string> -f [upper lower null] <address> <port>\n", program_name);
    fputs("Options:\n", stderr);
    fputs("  -h  Display this help message\n", stderr);
    fputs("  -s\t <string>  Input string to be converted\n", stderr);
    fputs("  -f\t <filter>  Choose from: [upper, lower, null]  \n", stderr);
    exit(exit_code);
}