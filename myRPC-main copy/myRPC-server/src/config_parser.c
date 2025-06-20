#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "config_parser.h"

Config parse_config(const char *filename) {
    Config config;
    config.port = 0;
    config.stream = 1;  // по умолчанию TCP

    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open config file");
        return config;
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = '\0';  // убрать \n

        if (line[0] == '#' || strlen(line) == 0)
            continue;

        char *key = strtok(line, "=");
        char *value = strtok(NULL, "");

        if (key && value) {
            if (strcmp(key, "port") == 0) {
                config.port = atoi(value);
            } else if (strcmp(key, "socket_type") == 0) {
                if (strcmp(value, "stream") == 0)
                    config.stream = 1;
                else if (strcmp(value, "dgram") == 0)
                    config.stream = 0;
            }
        }
    }

    fclose(file);
    return config;
}
