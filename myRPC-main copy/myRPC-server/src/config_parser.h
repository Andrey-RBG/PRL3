#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

typedef struct {
    int port;
    int stream;      // 1 = TCP (stream), 0 = UDP (datagram)
} Config;

Config parse_config(const char *filename);

#endif
