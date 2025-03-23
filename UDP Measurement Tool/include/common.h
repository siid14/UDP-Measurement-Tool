#ifndef COMMON_H
#define COMMON_H

// standard c libraries required for networking and time functions
#include <stdio.h>         // for input/output functions
#include <stdlib.h>        // for memory allocation and utility functions
#include <string.h>        // for string manipulation functions
#include <unistd.h>        // for unix standard functions
#include <sys/types.h>     // for system data types
#include <sys/socket.h>    // for socket functions
#include <netinet/in.h>    // for internet address structures
#include <arpa/inet.h>     // for ip address manipulation functions
#include <time.h>          // for time functions
#include <sys/time.h>      // for high precision time functions
#include <errno.h>         // for error handling
#include <signal.h>        // for signal handling

// default constants
#define DEFAULT_PORT 8888  // default udp port to use
#define TIMEOUT_SEC 2      // timeout in seconds for client waiting for response
#define TIMEOUT_USEC 0     // timeout in microseconds (additional precision)

// packet header structure definition
typedef struct {
    int seq_num;           // sequence number to identify each packet
    double timestamp;      // timestamp in seconds since epoch (send time)
    char data[0];          // flexible array member for payload data
} packet_header_t;

// function to get current time with microsecond precision
double get_current_time() {
    struct timeval tv;     // time value structure
    gettimeofday(&tv, NULL); // get current system time
    return tv.tv_sec + (tv.tv_usec / 1000000.0); // convert to seconds with microsecond precision
}

#endif