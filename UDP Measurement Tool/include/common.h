#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include <signal.h>

#define DEFAULT_PORT 8888
#define TIMEOUT_SEC 2      // Timeout in seconds for client waiting for response
#define TIMEOUT_USEC 0     // Timeout in microseconds

// Packet structure
typedef struct {
    int seq_num;           // Sequence number
    double timestamp;      // Timestamp in seconds since epoch
    char data[0];          // Flexible array member for payload
} packet_header_t;

// Get current time in seconds with microsecond precision
double get_current_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + (tv.tv_usec / 1000000.0);
}

#endif /* COMMON_H */ 