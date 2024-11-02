//
// Created by justinsaint on 01/11/24.
//

#ifndef SERVER_H
#define SERVER_H

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <netdb.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFFER_SIZE 1024
#define BASE_TEN 10
#define EXIT_CODE 1

static volatile sig_atomic_t exit_flag = 0;    // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

static void           setup_signal_handler(void);
static void           sigint_handler(int signum);
static void           parse_arguments(int argc, char *argv[], char **ip_address, char **port);
static void           handle_arguments(const char *binary_name, const char *ip_address, const char *port_str, in_port_t *port);
static in_port_t      parse_in_port_t(const char *binary_name, const char *port_str);
_Noreturn static void usage(const char *program_name, int exit_code, const char *message);
static void           convert_address(const char *address, struct sockaddr_storage *addr);
static int            socket_create(int domain, int type, int protocol);
static void           socket_bind(int sockfd, struct sockaddr_storage *addr, in_port_t port);
static void           start_listening(int server_fd, int backlog);
static int            socket_accept_connection(int server_fd, struct sockaddr_storage *client_addr, socklen_t *client_addr_len);
static void           handle_connection(int client_sockfd, struct sockaddr_storage *client_addr);
static void           shutdown_socket(int sockfd, int how);
static void           socket_close(int sockfd);

#endif //SERVER_H
