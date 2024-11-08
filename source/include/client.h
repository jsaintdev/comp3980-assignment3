//
// Created by justinsaint on 01/11/24.
//

#ifndef CLIENT_H
#define CLIENT_H

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define UNKNOWN_OPTION_MESSAGE_LEN 24
#define BUFFER_SIZE 1024
#define FILTER_SIZE 6
#define BASE_TWO 2
#define BASE_TEN 10

_Noreturn static void usage(const char *program_name, int exit_code, const char *message);
static void           parse_arguments(int argc, char *argv[], char *input_string, char *filter, char **address, char **port);
static void           handle_arguments(const char *binary_name, const char *input_string, const char *filter, const char *address, const char *port_str, in_port_t *port);
in_port_t             parse_in_port_t(const char *binary_name, const char *str);
static void           convert_address(const char *address, struct sockaddr_storage *addr);
static int            socket_create(int domain, int type, int protocol);
static void           socket_connect(int sockfd, struct sockaddr_storage *addr, in_port_t port);
static void           socket_close(int sockfd);
static void           send_data(int sockfd, const char *input_string, const char *filter);
static void           processed_data(int sockfd, char *buffer);

#endif    // CLIENT_H
