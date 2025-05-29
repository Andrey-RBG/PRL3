// myRPC-client/src/client.c — клиент с поддержкой JSON, аргументов и сокетов

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pwd.h>
#include "../../libmysyslog/libmysyslog.h"

#define BUF_SIZE 4096

void print_help(const char *progname) {
    printf("Usage: %s [OPTIONS]\n", progname);
    printf("Options:\n");
    printf("  -h, --host <ip>         Server IP address\n");
    printf("  -p, --port <port>       Server port\n");
    printf("  -s, --stream            Use stream (TCP) socket\n");
    printf("  -d, --dgram             Use datagram (UDP) socket\n");
    printf("  -c, --command <cmd>     Bash command to execute\n");
    printf("      --help              Show this help message\n");
}

int main(int argc, char *argv[]) {
    int opt;
    int option_index = 0;
    int use_stream = 1; // default
    char *host = NULL, *cmd = NULL;
    int port = -1;

    static struct option long_options[] = {
        {"help",    no_argument,       0,  0 },
        {"host",    required_argument, 0, 'h'},
        {"port",    required_argument, 0, 'p'},
        {"stream",  no_argument,       0, 's'},
        {"dgram",   no_argument,       0, 'd'},
        {"command", required_argument, 0, 'c'},
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv, "h:p:sdc:", long_options, &option_index)) != -1) {
        switch (opt) {
            case 0:
                if (strcmp(long_options[option_index].name, "help") == 0) {
                    print_help(argv[0]);
                    exit(EXIT_SUCCESS);
                }
                break;
            case 'h': host = optarg; break;
            case 'p': port = atoi(optarg); break;
            case 's': use_stream = 1; break;
            case 'd': use_stream = 0; break;
            case 'c': cmd = optarg; break;
            default:
                print_help(argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (!host || port <= 0 || !cmd) {
        fprintf(stderr, "[ERROR] Missing required arguments.\n");
        print_help(argv[0]);
        exit(EXIT_FAILURE);
    }

    struct passwd *pw = getpwuid(getuid());
    if (!pw) {
        perror("getpwuid");
        exit(EXIT_FAILURE);
    }

    char json[BUF_SIZE];
    snprintf(json, BUF_SIZE, "{\"login\":\"%s\",\"command\":\"%s\"}", pw->pw_name, cmd);

    int sockfd;
    struct sockaddr_in servaddr;

    if ((sockfd = socket(AF_INET, use_stream ? SOCK_STREAM : SOCK_DGRAM, 0)) < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    if (inet_pton(AF_INET, host, &servaddr.sin_addr) <= 0) {
        perror("inet_pton");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    if (use_stream && connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        perror("connect");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    ssize_t sent = sendto(sockfd, json, strlen(json), 0,
                          (struct sockaddr*)&servaddr, sizeof(servaddr));
    if (sent < 0) {
        perror("sendto");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    char buffer[BUF_SIZE];
    ssize_t recv_len = recvfrom(sockfd, buffer, BUF_SIZE - 1, 0, NULL, NULL);
    if (recv_len < 0) {
        perror("recvfrom");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    buffer[recv_len] = '\0';

    printf("Server response: %s\n", buffer);
    close(sockfd);
    return 0;
}
