//
// Created by justinsaint on 01/11/24.
//

include "server.h"

int main(int argc, char *argv[])
{
    char                   *input_string;
    char                   *filter;
    char                   *address;
    char                   *port_str;
    in_port_t               port;
    int                     sockfd;
    struct sockaddr_storage addr;

    input_string = NULL;
    filter = NULL;
    address  = NULL;
    port_str = NULL;

    // Set up the server
    parse_arguments(argc, argv, &address, &port_str);
    handle_arguments(argv[0], address, port_str, &port);
    convert_address(address, &addr);
    sockfd = socket_create(addr.ss_family, SOCK_STREAM, 0);
    socket_bind(sockfd, &addr, port);
    start_listening(sockfd, SOMAXCONN);

    // Set up Signal Handler
    setup_signal_handler();

    // Server Loop
    while(!(exit_flag))
    {
        int                     client_sockfd;
        struct sockaddr_storage client_addr;
        socklen_t               client_addr_len;
        pid_t                   pid;

        client_addr_len = sizeof(client_addr);
        client_sockfd   = socket_accept_connection(sockfd, &client_addr, &client_addr_len);

        if(client_sockfd == -1)
        {
            if(exit_flag)
            {
                break;
            }
            continue;
        }

        // Fork a new child process for each new connection
        pid = fork();
        if(pid == -1)
        {
            perror("Error creating child process");
            close(client_sockfd);
            continue;
        }

        if(pid == 0)
        {
            // Child Process
            close(sockfd);

            // Receive, process, and send data back
            receive_data(client_sockfd, &client_addr, input_string, filter);
            process_string(input_string, filter);
            send_data(client_sockfd, input_string, filter);

            // Shut down child process
            shutdown_socket(client_sockfd, SHUT_RDWR);
            socket_close(client_sockfd);
            exit(EXIT_SUCCESS);
        }
        else
        {
            // Parent Process
            close(client_sockfd);
            waitpid(-1, NULL, WNOHANG);
        }
    }

    // Graceful Termination
    shutdown_socket(sockfd, SHUT_RDWR);
    socket_close(sockfd);

    return EXIT_SUCCESS;
}

static void parse_arguments(int argc, char *argv[], char **ip_address, char **port)
{
    int opt;

    opterr = 0;

    while((opt = getopt(argc, argv, "h")) != -1)
    {
        switch(opt)
        {
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

    if(optind >= argc)
    {
        usage(argv[0], EXIT_FAILURE, "The ip address and port are required");
    }

    if(optind + 1 >= argc)
    {
        usage(argv[0], EXIT_FAILURE, "The port is required");
    }

    if(optind < argc - 2)
    {
        usage(argv[0], EXIT_FAILURE, "Error: Too many arguments.");
    }

    *ip_address = argv[optind];
    *port       = argv[optind + 1];
}

static void handle_arguments(const char *binary_name, const char *ip_address, const char *port_str, in_port_t *port)
{
    if(ip_address == NULL)
    {
        usage(binary_name, EXIT_FAILURE, "The ip address is required.");
    }

    if(port_str == NULL)
    {
        usage(binary_name, EXIT_FAILURE, "The port is required.");
    }

    *port = parse_in_port_t(binary_name, port_str);
}

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

_Noreturn static void usage(const char *program_name, int exit_code, const char *message)
{
    if(message)
    {
        fprintf(stderr, "%s\n", message);
    }

    fprintf(stderr, "Usage: %s [-h] <ip address> <port>\n", program_name);
    fputs("Options:\n", stderr);
    fputs("  -h  Display this help message\n", stderr);
    exit(exit_code);
}

static void setup_signal_handler(void)
{
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));
#if defined(__clang__)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wdisabled-macro-expansion"
#endif
    sa.sa_handler = sigint_handler;
#if defined(__clang__)
    #pragma clang diagnostic pop
#endif
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if(sigaction(SIGINT, &sa, NULL) == -1)
    {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

static void sigint_handler(int signum)
{
    exit_flag = 1;
}

#pragma GCC diagnostic pop

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

static void socket_bind(int sockfd, struct sockaddr_storage *addr, in_port_t port)
{
    char      addr_str[INET6_ADDRSTRLEN];
    socklen_t addr_len;
    void     *vaddr;
    in_port_t net_port;

    net_port = htons(port);

    if(addr->ss_family == AF_INET)
    {
        struct sockaddr_in *ipv4_addr;

        ipv4_addr           = (struct sockaddr_in *)addr;
        addr_len            = sizeof(*ipv4_addr);
        ipv4_addr->sin_port = net_port;
        vaddr               = (void *)&(((struct sockaddr_in *)addr)->sin_addr);
    }
    else if(addr->ss_family == AF_INET6)
    {
        struct sockaddr_in6 *ipv6_addr;

        ipv6_addr            = (struct sockaddr_in6 *)addr;
        addr_len             = sizeof(*ipv6_addr);
        ipv6_addr->sin6_port = net_port;
        vaddr                = (void *)&(((struct sockaddr_in6 *)addr)->sin6_addr);
    }
    else
    {
        fprintf(stderr, "Internal error: addr->ss_family must be AF_INET or AF_INET6, was: %d\n", addr->ss_family);
        exit(EXIT_FAILURE);
    }

    if(inet_ntop(addr->ss_family, vaddr, addr_str, sizeof(addr_str)) == NULL)
    {
        perror("inet_ntop");
        exit(EXIT_FAILURE);
    }

    printf("Binding to: %s:%u\n", addr_str, port);

    if(bind(sockfd, (struct sockaddr *)addr, addr_len) == -1)
    {
        perror("Binding failed");
        fprintf(stderr, "Error code: %d\n", errno);
        exit(EXIT_FAILURE);
    }

    printf("Bound to socket: %s:%u\n", addr_str, port);
}

static void start_listening(int server_fd, int backlog)
{
    if(listen(server_fd, backlog) == -1)
    {
        perror("listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Listening for incoming connections...\n");
}

static int socket_accept_connection(int server_fd, struct sockaddr_storage *client_addr, socklen_t *client_addr_len)
{
    int  client_fd;
    char client_host[NI_MAXHOST];
    char client_service[NI_MAXSERV];

    errno     = 0;
    client_fd = accept(server_fd, (struct sockaddr *)client_addr, client_addr_len);

    if(client_fd == -1)
    {
        if(errno != EINTR)
        {
            perror("accept failed");
        }

        return -1;
    }

    if(getnameinfo((struct sockaddr *)client_addr, *client_addr_len, client_host, NI_MAXHOST, client_service, NI_MAXSERV, 0) == 0)
    {
        printf("Accepted a new connection from %s:%s\n", client_host, client_service);
    }
    else
    {
        printf("Unable to get client information\n");
    }

    return client_fd;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

// cppcheck-suppress constParameterPointer
static void receive_data(int client_sockfd, struct sockaddr_storage *client_addr, char *input_string, char *filter)
{
    ssize_t read_bytes;
    uint8_t filter_len;
    uint8_t string_len;

    // Receive the filter
    read_bytes = read(client_sockfd, &filter_len, sizeof(filter_len));
    if(read_bytes <= 0)
    {
        perror("Error reading filter length from socket");
        exit(EXIT_FAILURE);
    }

    read_bytes = read(client_sockfd, filter, filter_len);
    if(read_bytes <= 0)
    {
        perror("Error reading filter from socket");
        exit(EXIT_FAILURE);
    }
    filter[filter_len] = '\0';

    // Receive the string
    read_bytes = read(client_sockfd, &string_len, sizeof(string_len));
    if(read_bytes <= 0)
    {
        perror("Error reading string length from socket");
        exit(EXIT_FAILURE);
    }

    read_bytes = read(client_sockfd, input_string, string_len);
    if(read_bytes <= 0)
    {
        perror("Error reading input string from socket");
        exit(EXIT_FAILURE);
    }
    input_string[string_len] = '\0';
}

#pragma GCC diagnostic pop

static void shutdown_socket(int sockfd, int how)
{
    if(shutdown(sockfd, how) == -1)
    {
        perror("Error closing socket");
        exit(EXIT_FAILURE);
    }
}

static void socket_close(int sockfd)
{
    if(close(sockfd) == -1)
    {
        perror("Error closing socket");
        exit(EXIT_FAILURE);
    }
}

// Apply the chosen filter on the string
static void process_string(char *input_string, const char *filter)
{
    if(strcmp(filter, "upper") == 0)
    {
        for(int i = 0; input_string[i]; i++)
        {
            input_string[i] = (char)toupper((unsigned char)input_string[i]);
        }
    }
    else if(strcmp(filter, "lower") == 0)
    {
        for(int i = 0; input_string[i]; i++)
        {
            input_string[i] = (char)tolower((unsigned char)input_string[i]);
        }
    }
    else if(strcmp(filter, "null") == 0)
    {
        printf("Filter is null. No transformation applied.\n");
    }
}

// Send data to server
static void send_data(int sockfd, const char *input_string)
{
    ssize_t write_bytes;
    uint8_t string_len = (uint8_t)strlen(input_string);

    write_bytes = write(sockfd, &string_len, sizeof(uint8_t));
    if(write_bytes < 0)
    {
        perror("Error writing string length to socket");
        exit(EXIT_FAILURE);
    }

    write_bytes = write(sockfd, input_string, string_len);
    if(write_bytes < 0)
    {
        perror("Error writing string to socket");
        exit(EXIT_FAILURE);
    }
}