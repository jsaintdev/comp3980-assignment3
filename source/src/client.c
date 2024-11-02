//
// Created by justinsaint on 01/11/24.
//

include "client.h"

int main(int argc, char *argv[])
{
    char                   *address;
    char                   *port_str;
    char                   *input_string;
    char                   *filter;
    in_port_t               port;
    int                     sockfd;
    struct sockaddr_storage addr;

    address  = NULL;
    port_str = NULL;
    input_string = NULL;
    filter = NULL;
    parse_arguments(argc, argv, &input_string, &filter, &address, &port_str);
    handle_arguments(argv[0], input_string, filter, address, port_str, &port);

    convert_address(address, &addr);
    sockfd = socket_create(addr.ss_family, SOCK_STREAM, 0);
    socket_connect(sockfd, &addr, port);
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

    if(optind >= argc - BASE_ONE)
    {
        usage(argv[0], EXIT_FAILURE, "Too few arguments.");
    }

    if(optind < argc - BASE_TWO)
    {
        usage(argv[0], EXIT_FAILURE, "Too many arguments.");
    }

    *address = argv[optind];
    *port    = argv[optind + BASE_ONE];
}

// Checks if the input was valid
static void handle_arguments(const char *binary_name, const char *input_string, const char *filter, const char *address, const char *port_str, in_port_t *port)
{
    if(input_string == NULL)
    {
        usage(binary_name, EXIT_FAILURE, "A string is required.");
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