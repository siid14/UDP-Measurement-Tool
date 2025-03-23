#include "include/common.h"

// Global variables
int server_socket;
FILE *log_file;
double total_owd = 0.0;
int packet_count = 0;

void cleanup_and_exit(int sig) {
    if (packet_count > 0) {
        double avg_owd = total_owd / packet_count;
        printf("\nStatistics:\n");
        printf("Packets received: %d\n", packet_count);
        printf("Average OWD: %.6f seconds\n", avg_owd);
        fprintf(log_file, "Summary: Packets received: %d, Average OWD: %.6f seconds\n", 
                packet_count, avg_owd);
    } else {
        printf("\nNo packets received\n");
    }
    
    if (log_file) fclose(log_file);
    if (server_socket) close(server_socket);
    exit(0);
}

int main(int argc, char *argv[]) {
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    int port = DEFAULT_PORT;
    char client_ip[INET_ADDRSTRLEN];
    
    // Parse command line arguments
    if (argc > 1) {
        port = atoi(argv[1]);
        if (port <= 0 || port > 65535) {
            fprintf(stderr, "Invalid port number. Using default port %d\n", DEFAULT_PORT);
            port = DEFAULT_PORT;
        }
    }
    
    // Setup signal handler for graceful exit
    signal(SIGINT, cleanup_and_exit);
    
    // Open log file
    log_file = fopen("server_log.txt", "w");
    if (log_file == NULL) {
        perror("Failed to open log file");
        exit(EXIT_FAILURE);
    }
    
    // Create socket
    server_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_socket < 0) {
        perror("Socket creation failed");
        fclose(log_file);
        exit(EXIT_FAILURE);
    }
    
    // Set up server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);
    
    // Bind the socket
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_socket);
        fclose(log_file);
        exit(EXIT_FAILURE);
    }
    
    printf("UDP Server running on port %d...\n", port);
    fprintf(log_file, "UDP Server started on port %d\n", port);
    
    while (1) {
        // Buffer for received packet
        char buffer[65507]; // Max UDP packet size
        int recv_len;
        
        // Receive packet
        recv_len = recvfrom(server_socket, buffer, sizeof(buffer), 0,
                         (struct sockaddr *)&client_addr, &client_len);
        
        if (recv_len < 0) {
            perror("Receive failed");
            continue;
        }
        
        // Calculate current time (for OWD calculation)
        double receive_time = get_current_time();
        
        // Parse packet header
        packet_header_t *header = (packet_header_t *)buffer;
        double send_time = header->timestamp;
        int seq_num = header->seq_num;
        
        // Calculate one-way delay (OWD)
        double owd = receive_time - send_time;
        total_owd += owd;
        packet_count++;
        
        // Get client IP as string
        inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
        
        // Log packet info
        printf("Received packet #%d from %s:%d, OWD: %.6f seconds\n",
               seq_num, client_ip, ntohs(client_addr.sin_port), owd);
        
        fprintf(log_file, "Received packet #%d from %s:%d, OWD: %.6f seconds\n",
                seq_num, client_ip, ntohs(client_addr.sin_port), owd);
        
        // Echo the packet back to client
        if (sendto(server_socket, buffer, recv_len, 0,
                  (struct sockaddr *)&client_addr, client_len) < 0) {
            perror("Send failed");
        }
    }
    
    // This code is never reached due to the infinite loop, but added for completeness
    close(server_socket);
    fclose(log_file);
    
    return 0;
} 