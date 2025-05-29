#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include <fcntl.h>
#include "config_parser.h"
#include "../../libmysyslog/libmysyslog.h"

#define BUF_SIZE 4096
#define STDOUT_TEMPLATE "/tmp/myRPC_XXXXXX.stdout"
#define STDERR_TEMPLATE "/tmp/myRPC_XXXXXX.stderr"
#define USERS_CONF "/etc/myRPC/users.conf"

static char *trim_whitespace(char *str) {
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return str;

    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';

    return str;
}

static int is_user_allowed(const char *username) {
    FILE *f = fopen(USERS_CONF, "r");
    if (!f) {
        log_error("Cannot open users config file: %s", USERS_CONF);
        return 0;
    }
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        char *name = trim_whitespace(line);
        if (name[0] == '#' || name[0] == '\0') continue;
        if (strcmp(name, username) == 0) {
            fclose(f);
            return 1;
        }
    }
    fclose(f);
    return 0;
}

static void handle_request(const char *json, int sockfd, struct sockaddr_in *client_addr, socklen_t addrlen) {
    char user[256] = {0}, cmd[1024] = {0};

    char *login_pos = strstr(json, "\"login\":\"");
    char *cmd_pos = strstr(json, "\"command\":\"");
    if (!login_pos || !cmd_pos) {
        const char *resp = "{\"code\":1,\"result\":\"Invalid JSON\"}";
        sendto(sockfd, resp, strlen(resp), 0, (struct sockaddr*)client_addr, addrlen);
        log_error("Invalid JSON received: %s", json);
        return;
    }
    sscanf(login_pos, "\"login\":\"%255[^\"]", user);
    sscanf(cmd_pos, "\"command\":\"%1023[^\"]", cmd);

    log_info("Request from user='%s', command='%s'", user, cmd);

    if (!is_user_allowed(user)) {
        const char *resp = "{\"code\":1,\"result\":\"User not allowed\"}";
        sendto(sockfd, resp, strlen(resp), 0, (struct sockaddr*)client_addr, addrlen);
        log_warn("Unauthorized access attempt by user: %s", user);
        return;
    }

    char stdout_tpl[] = STDOUT_TEMPLATE;
    char stderr_tpl[] = STDERR_TEMPLATE;
    int outfd = mkstemps(stdout_tpl, 7);
    int errfd = mkstemps(stderr_tpl, 7);

    if (outfd < 0 || errfd < 0) {
        const char *resp = "{\"code\":1,\"result\":\"Temp file error\"}";
        sendto(sockfd, resp, strlen(resp), 0, (struct sockaddr*)client_addr, addrlen);
        log_error("Failed to create temp files");
        return;
    }

    pid_t pid = fork();
    if (pid == 0) {
        dup2(outfd, STDOUT_FILENO);
        dup2(errfd, STDERR_FILENO);
        close(outfd);
        close(errfd);

        execl("/bin/bash", "bash", "-c", cmd, NULL);
        exit(127);
    } else if (pid > 0) {
        close(outfd);
        close(errfd);
        int status;
        waitpid(pid, &status, 0);

        FILE *f = fopen(stdout_tpl, "r");
        if (!f) {
            const char *resp = "{\"code\":1,\"result\":\"Failed to open stdout\"}";
            sendto(sockfd, resp, strlen(resp), 0, (struct sockaddr*)client_addr, addrlen);
            log_error("Cannot open temp stdout file: %s", stdout_tpl);
            unlink(stdout_tpl);
            unlink(stderr_tpl);
            return;
        }

        char result[BUF_SIZE] = {0};
        size_t r = fread(result, 1, BUF_SIZE - 1, f);
        fclose(f);
        unlink(stdout_tpl);
        unlink(stderr_tpl);

        char response[BUF_SIZE];
        snprintf(response, sizeof(response), "{\"code\":0,\"result\":\"%.*s\"}", (int)r, result);
        sendto(sockfd, response, strlen(response), 0, (struct sockaddr*)client_addr, addrlen);

        log_info("Command executed for user='%s', exit_code=%d", user, WEXITSTATUS(status));
    } else {
        log_error("Fork failed: %s", strerror(errno));
        const char *resp = "{\"code\":1,\"result\":\"Server error\"}";
        sendto(sockfd, resp, strlen(resp), 0, (struct sockaddr*)client_addr, addrlen);
    }
}

int main() {
    Config conf = parse_config("/etc/myRPC/myRPC.conf");
    if (conf.port == 0) {
        log_fatal("Invalid or missing port in config");
        exit(EXIT_FAILURE);
    }

    int sockfd = socket(AF_INET, conf.stream ? SOCK_STREAM : SOCK_DGRAM, 0);
    if (sockfd < 0) {
        log_fatal("socket: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in servaddr = {0};
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(conf.port);

    if (bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        log_fatal("bind: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (conf.stream) {
        if (listen(sockfd, 5) < 0) {
            log_fatal("listen: %s", strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    log_info("myRPC-server listening on port %d (%s)", conf.port, conf.stream ? "TCP" : "UDP");

    while (1) {
        char buffer[BUF_SIZE];
        struct sockaddr_in clientaddr;
        socklen_t addrlen = sizeof(clientaddr);

        if (conf.stream) {
            int connfd = accept(sockfd, (struct sockaddr*)&clientaddr, &addrlen);
            if (connfd < 0) {
                log_error("accept failed: %s", strerror(errno));
                continue;
            }
            ssize_t n = recv(connfd, buffer, BUF_SIZE - 1, 0);
            if (n > 0) {
                buffer[n] = '\0';
                handle_request(buffer, connfd, &clientaddr, addrlen);
            }
            close(connfd);
        } else {
            ssize_t n = recvfrom(sockfd, buffer, BUF_SIZE - 1, 0, (struct sockaddr*)&clientaddr, &addrlen);
            if (n > 0) {
                buffer[n] = '\0';
                handle_request(buffer, sockfd, &clientaddr, addrlen);
            }
        }
    }

    close(sockfd);
    return 0;
}
