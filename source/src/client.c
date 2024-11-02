//
// Created by justinsaint on 01/11/24.
//

include "client.h"

int main(int argc, char *argv[])
{
    char                   *input_string;
    char                   *filter;
    char                   *address;
    char                   *port_str;
    char                    buffer[BUFFER_SIZE];
    in_port_t               port;
    int                     sockfd;
    struct sockaddr_storage addr;

    input_string = NULL;
    filter = NULL;
    address  = NULL;
    port_str = NULL;

    // Parse and handle user input
    parse_arguments(argc, argv, &input_string, &filter, &address, &port_str);
    handle_arguments(argv[0], input_string, filter, address, port_str, &port);

    // Set up network socket
    convert_address(address, &addr);
    sockfd = socket_create(addr.ss_family, SOCK_STREAM, 0);
    socket_connect(sockfd, &addr, port);

    // Send data to server
    send_data(sockfd, input_string, filter);
    processed_date(sockfd, buffer);

    socket_close(sockfd);
    return EXIT_SUCCESS;
}

// Displays a usage message for the user
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

// Parses user input and checks if the correct number of arguments were passed
static void parse_arguments(int argc, char *argv[], char **input_string, char **filter, char **address, char **port)
{
    int opt;

    opterr = 0;

    while((opt = getopt(argc, argv, "hs:f:")) != -1)
    {
        switch(opt)
        {
            case 's':
            {
                *input_string = optarg;
                break;
            }
            case 'f':
            {
                *filter = optarg;
                break;
            }
            case 'h':
            {
                usage(argv[0], EXIT_SUCCESS, NULL);
            }
            case '?':
            {
                char message[UNKNOWN_OPTION_MESSAGE_LEN];

                snprintf(message, sizeof(message), "Unknown option '-%c'.", optopt);
                usage(argv[0], EXIT_FAILURE, message);
            }
            default:
            {
                usage(argv[0], EXIT_FAILURE, NULL);
            }
        }
    }

    if(optind >= argc - 1)
    {
        usage(argv[0], EXIT_FAILURE, "Too few arguments.");
    }

    if(optind < argc - BASE_TWO)
    {
        usage(argv[0], EXIT_FAILURE, "Too many arguments.");
    }

    *address = argv[optind];
    *port    = argv[optind + 1];
}

// Checks if the input was valid
static void handle_arguments(const char *binary_name, const char *input_string, const char *filter, const char *address, const char *port_str, in_port_t *port)
{
    if(input_string == NULL)
    {
        usage(binary_name, EXIT_FAILURE, "A string is required.");
    }

    if(strlen(input_string) >= BUFFER_SIZE)
    {
        usage(binary_name, EXIT_FAILURE, "Input string exceeds maximum length of %d characters", BUFFER_SIZE - ONE);
    }

    if(filter == NULL || (strcmp(filter, "upper") != 0 && strcmp(filter, "lower") != 0 && strcmp(filter, "null") != 0))
    {
        usage(binary_name, EXIT_FAILURE, "A valid filter is required.");
    }

    if(address == NULL)
    {
        usage(binary_name, EXIT_FAILURE, "The port is required.");
    }

    if(port_str == NULL)
    {
        usage(binary_name, EXIT_FAILURE, "The address is required.");
    }

    *port = parse_in_port_t(binary_name, port_str);
}

// Parses user input and validates the port number for the socket
in_port_t parse_in_port_t(const char *binary_name, const char *str)
{
    char     *endptr;
    uintmax_t parsed_value;

    errno        = 0;
    parsed_value = strtoumax(str, &endptr, BASE_TEN);

    if(errno != 0)
    {
        perror("Error parsing in_port_t");
        exit(EXIT_FAILURE);
    }

    // Check if there are any non-numeric characters in the input string
    if(*endptr != '\0')
    {
        usage(binary_name, EXIT_FAILURE, "Invalid characters in input.");
    }

    // Check if the parsed value is within the valid range for in_port_t
    if(parsed_value > UINT16_MAX)
    {
        usage(binary_name, EXIT_FAILURE, "in_port_t value out of range.");
    }

    return (in_port_t)parsed_value;
}

// Converts user input into binary to set up the socket address
static void convert_address(const char *address, struct sockaddr_storage *addr)
{
    memset(addr, 0, sizeof(*addr));

    if(inet_pton(AF_INET, address, &(((struct sockaddr_in *)addr)->sin_addr)) == 1)
    {
        addr->ss_family = AF_INET;
    }
    else if(inet_pton(AF_INET6, address, &(((struct sockaddr_in6 *)addr)->sin6_addr)) == 1)
    {
        addr->ss_family = AF_INET6;
    }
    else
    {
        fprintf(stderr, "%s is not an IPv4 or an IPv6 address\n", address);
        exit(EXIT_FAILURE);
    }
}

// Create the network socket for client
static int socket_create(int domain, int type, int protocol)
{
    int sockfd;
    sockfd = socket(domain, type, protocol);

    if(sockfd == -1)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    return sockfd;
}

// Establishes a connection between the client and the server
static void socket_connect(int sockfd, struct sockaddr_storage *addr, in_port_t port)
{
    char      addr_str[INET6_ADDRSTRLEN];
    in_port_t net_port;
    socklen_t addr_len;

    if(inet_ntop(addr->ss_family, addr->ss_family == AF_INET ? (void *)&(((struct sockaddr_in *)addr)->sin_addr) : (void *)&(((struct sockaddr_in6 *)addr)->sin6_addr), addr_str, sizeof(addr_str)) == NULL)
    {
        perror("inet_ntop");
        exit(EXIT_FAILURE);
    }

    printf("Connecting to: %s:%u\n", addr_str, port);
    net_port = htons(port);

    if(addr->ss_family == AF_INET)
    {
        struct sockaddr_in *ipv4_addr;

        ipv4_addr           = (struct sockaddr_in *)addr;
        ipv4_addr->sin_port = net_port;
        addr_len            = sizeof(struct sockaddr_in);
    }
    else if(addr->ss_family == AF_INET6)
    {
        struct sockaddr_in6 *ipv6_addr;

        ipv6_addr            = (struct sockaddr_in6 *)addr;
        ipv6_addr->sin6_port = net_port;
        addr_len             = sizeof(struct sockaddr_in6);
    }
    else
    {
        fprintf(stderr, "Invalid address family: %d\n", addr->ss_family);
        exit(EXIT_FAILURE);
    }

    if(connect(sockfd, (struct sockaddr *)addr, addr_len) == -1)
    {
        const char *msg;

        msg = strerror(errno);
        fprintf(stderr, "Error: connect (%d): %s\n", errno, msg);
        exit(EXIT_FAILURE);
    }

    printf("Connected to: %s:%u\n", addr_str, port);
}

// Closes the socket
static void socket_close(int sockfd)
{
    if(close(sockfd) == -1)
    {
        perror("Error closing socket");
        exit(EXIT_FAILURE);
    }
}

// Send data to server
static void send_data(int sockfd, const char *input_string, const char *filter)
{
    // Send the filter
    uint8_t filter_len = (uint8_t)strlen(filter);
    write(sockfd, &filter_len, sizeof(uint8_t));
    write(sockfd, filter, filter_len);

    // Send the string
    uint8_t string_len = (uint8_t)strlen(input_string);
    write(sockfd, &string_len, sizeof(uint8_t));
    write(sockfd, input_string, string_len);
}

// Receives processed string and prints it to console
static void processed_data(int sockfd, char *buffer)
{
    uint8_t length;
    ssize_t read_bytes;

    // Read the length of the processed string
    read_bytes = read(sockfd, &length, sizeof(length));

    if(read_bytes < 0)
    {
        perror("Error reading processed string length from socket");
        exit(EXIT_FAILURE);
    }

    if(length == 0)
    {
        break;
    }

    // Read the processed string
    read_bytes = read(sockfd, buffer, length);

    if(read_bytes < 0)
    {
        perror("Error reading processed string from socket");
        exit(EXIT_FAILURE);
    }

    buffer[length] = '\0';
    printf("Processed string from server: \n%s\n", buffer);
}