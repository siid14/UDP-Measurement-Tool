#include "include/common.h"

// global variables
int server_socket;        // socket file descriptor
FILE *log_file;           // file pointer for logging
double total_owd = 0.0;   // sum of all one-way delays
int packet_count = 0;     // counter for received packets

// function to handle graceful exit and print statistics
void cleanup_and_exit(int sig) {
    if (packet_count > 0) {
        // calculate average one-way delay
        double avg_owd = total_owd / packet_count;
        printf("\nStatistics:\n");
        printf("Packets received: %d\n", packet_count);
        printf("Average OWD: %.6f seconds\n", avg_owd);
        fprintf(log_file, "Summary: Packets received: %d, Average OWD: %.6f seconds\n", 
                packet_count, avg_owd);
    } else {
        printf("\nNo packets received\n");
    }
    
    // close files and sockets
    if (log_file) fclose(log_file);
    if (server_socket) close(server_socket);
    exit(0);
}

int main(int argc, char *argv[]) {
    struct sockaddr_in server_addr, client_addr;  // socket address structures
    socklen_t client_len = sizeof(client_addr);   // size of client address structure
    int port = DEFAULT_PORT;                      // port number to listen on
    char client_ip[INET_ADDRSTRLEN];              // buffer for client ip string
    
    // parse command line arguments for custom port
    if (argc > 1) {
        port = atoi(argv[1]);
        if (port <= 0 || port > 65535) {
            fprintf(stderr, "Invalid port number. Using default port %d\n", DEFAULT_PORT);
            port = DEFAULT_PORT;
        }
    }
    
    // setup signal handler for ctrl+c
    signal(SIGINT, cleanup_and_exit);
    
    // open log file for writing
    log_file = fopen("server_log.txt", "w");
    if (log_file == NULL) {
        perror("Failed to open log file");
        exit(EXIT_FAILURE);
    }
    
    // create udp socket
    server_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_socket < 0) {
        perror("Socket creation failed");
        fclose(log_file);
        exit(EXIT_FAILURE);
    }
    
    // initialize server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;             // ipv4
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // listen on all interfaces
    server_addr.sin_port = htons(port);           // set port in network byte order
    
    // bind socket to the specified address and port
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_socket);
        fclose(log_file);
        exit(EXIT_FAILURE);
    }
    
    // server started successfully
    printf("UDP Server running on port %d...\n", port);
    fprintf(log_file, "UDP Server started on port %d\n", port);
    
    // main server loop
    while (1) {
        // buffer for received packet
        char buffer[65507]; // max udp packet size
        int recv_len;
        
        // receive a packet from any client
        recv_len = recvfrom(server_socket, buffer, sizeof(buffer), 0,
                         (struct sockaddr *)&client_addr, &client_len);
        
        if (recv_len < 0) {
            perror("Receive failed");
            continue;
        }
        
        // get current time for owd calculation
        double receive_time = get_current_time();
        
        // extract header information from the packet
        packet_header_t *header = (packet_header_t *)buffer;
        double send_time = header->timestamp;
        int seq_num = header->seq_num;
        
        // calculate one-way delay (owd)
        double owd = receive_time - send_time;
        total_owd += owd;
        packet_count++;
        
        // convert client ip address to string
        inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
        
        // print and log packet info
        printf("Received packet #%d from %s:%d, OWD: %.6f seconds\n",
               seq_num, client_ip, ntohs(client_addr.sin_port), owd);
        
        fprintf(log_file, "Received packet #%d from %s:%d, OWD: %.6f seconds\n",
                seq_num, client_ip, ntohs(client_addr.sin_port), owd);
        
        // echo the packet back to client for rtt measurement
        if (sendto(server_socket, buffer, recv_len, 0,
                  (struct sockaddr *)&client_addr, client_len) < 0) {
            perror("Send failed");
        }
    }
    
    // ! never reached due to the infinite loop, but added for completeness
    close(server_socket);
    fclose(log_file);
    
    return 0;
} 