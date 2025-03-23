#include "include/common.h"

int main(int argc, char *argv[]) {
    int client_socket;
    struct sockaddr_in server_addr;
    struct timeval tv;
    char server_ip[INET_ADDRSTRLEN];
    int port = DEFAULT_PORT;
    int num_packets = 10;
    int message_size = 64;
    double inter_packet_delay = 1.0;
    char *payload = NULL;
    int total_size;
    FILE *log_file;
    double total_rtt = 0.0;
    int received_packets = 0;
    fd_set read_fds;
    double rtt_values[10000]; // Assuming not more than 10000 packets
    int timeout_count = 0;
    
    // Parse command line arguments
    printf("UDP Client for network delay measurement\n");
    
    if (argc < 2) {
        printf("Enter server IP address: ");
        scanf("%s", server_ip);
    } else {
        strncpy(server_ip, argv[1], INET_ADDRSTRLEN);
    }
    
    if (argc < 3) {
        printf("Enter server port [%d]: ", DEFAULT_PORT);
        char input[32];
        fgets(input, sizeof(input), stdin);
        if (strlen(input) > 1) {
            port = atoi(input);
            if (port <= 0 || port > 65535) port = DEFAULT_PORT;
        }
    } else {
        port = atoi(argv[2]);
        if (port <= 0 || port > 65535) port = DEFAULT_PORT;
    }
    
    printf("Enter number of packets to send: ");
    scanf("%d", &num_packets);
    if (num_packets <= 0) num_packets = 10;
    
    printf("Enter message size in bytes (min %ld): ", sizeof(packet_header_t));
    scanf("%d", &message_size);
    if (message_size < (int)sizeof(packet_header_t)) message_size = sizeof(packet_header_t);
    
    printf("Enter inter-packet delay in seconds: ");
    scanf("%lf", &inter_packet_delay);
    if (inter_packet_delay < 0) inter_packet_delay = 0;
    
    // Open log file
    log_file = fopen("client_log.txt", "w");
    if (log_file == NULL) {
        perror("Failed to open log file");
        exit(EXIT_FAILURE);
    }
    
    // Create socket
    client_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_socket < 0) {
        perror("Socket creation failed");
        fclose(log_file);
        exit(EXIT_FAILURE);
    }
    
    // Set receive timeout
    tv.tv_sec = TIMEOUT_SEC;
    tv.tv_usec = TIMEOUT_USEC;
    if (setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("Error setting socket timeout");
    }
    
    // Set up server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("Invalid address");
        close(client_socket);
        fclose(log_file);
        exit(EXIT_FAILURE);
    }
    
    fprintf(log_file, "UDP Client started, connecting to %s:%d\n", server_ip, port);
    fprintf(log_file, "Sending %d packets of %d bytes with %.2f seconds delay\n", 
            num_packets, message_size, inter_packet_delay);
    
    // Allocate memory for the packet
    total_size = message_size;
    char *buffer = (char *)malloc(total_size);
    if (!buffer) {
        perror("Memory allocation failed");
        close(client_socket);
        fclose(log_file);
        exit(EXIT_FAILURE);
    }
    
    // Create payload data (if any)
    int payload_size = message_size - sizeof(packet_header_t);
    if (payload_size > 0) {
        payload = (char *)malloc(payload_size);
        if (!payload) {
            perror("Memory allocation failed");
            free(buffer);
            close(client_socket);
            fclose(log_file);
            exit(EXIT_FAILURE);
        }
        
        // Fill payload with random data
        for (int i = 0; i < payload_size; i++) {
            payload[i] = 'A' + (i % 26);
        }
    }
    
    printf("Sending %d packets to %s:%d\n", num_packets, server_ip, port);
    
    // Send packets
    for (int i = 0; i < num_packets; i++) {
        packet_header_t *header = (packet_header_t *)buffer;
        header->seq_num = i + 1;
        header->timestamp = get_current_time();
        
        // Add payload if necessary
        if (payload_size > 0) {
            memcpy(buffer + sizeof(packet_header_t), payload, payload_size);
        }
        
        // Send packet
        if (sendto(client_socket, buffer, total_size, 0,
                  (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
            perror("Send failed");
            continue;
        }
        
        printf("Sent packet #%d\n", i + 1);
        
        // Wait for response with timeout
        FD_ZERO(&read_fds);
        FD_SET(client_socket, &read_fds);
        
        struct timeval wait_time;
        wait_time.tv_sec = TIMEOUT_SEC;
        wait_time.tv_usec = TIMEOUT_USEC;
        
        int activity = select(client_socket + 1, &read_fds, NULL, NULL, &wait_time);
        
        if (activity < 0) {
            perror("Select error");
        } else if (activity == 0) {
            // Timeout occurred
            printf("Timeout: Packet #%d response not received\n", i + 1);
            fprintf(log_file, "Timeout: Packet #%d response not received\n", i + 1);
            timeout_count++;
        } else {
            // Data available, receive it
            socklen_t len = sizeof(server_addr);
            int recv_len = recvfrom(client_socket, buffer, total_size, 0,
                              (struct sockaddr *)&server_addr, &len);
            
            if (recv_len < 0) {
                perror("Receive failed");
                fprintf(log_file, "Receive failed for packet #%d: %s\n", i + 1, strerror(errno));
            } else {
                // Parse received packet
                packet_header_t *resp_header = (packet_header_t *)buffer;
                double receive_time = get_current_time();
                double send_time = resp_header->timestamp;
                int seq_num = resp_header->seq_num;
                
                // Calculate RTT
                double rtt = receive_time - send_time;
                rtt_values[received_packets] = rtt;
                total_rtt += rtt;
                received_packets++;
                
                printf("Received response for packet #%d, RTT: %.6f seconds\n", seq_num, rtt);
                fprintf(log_file, "Packet #%d, RTT: %.6f seconds\n", seq_num, rtt);
            }
        }
        
        // Introduce delay between packets if requested
        if (inter_packet_delay > 0 && i < num_packets - 1) {
            usleep((useconds_t)(inter_packet_delay * 1000000));
        }
    }
    
    // Calculate statistics
    double packet_loss = (num_packets - received_packets) * 100.0 / num_packets;
    
    printf("\nStatistics:\n");
    printf("Packets sent: %d\n", num_packets);
    printf("Packets received: %d\n", received_packets);
    printf("Packet loss: %.2f%%\n", packet_loss);
    
    if (received_packets > 0) {
        double avg_rtt = total_rtt / received_packets;
        printf("Average RTT: %.6f seconds\n", avg_rtt);
        
        fprintf(log_file, "\nSummary:\n");
        fprintf(log_file, "Packets sent: %d\n", num_packets);
        fprintf(log_file, "Packets received: %d\n", received_packets);
        fprintf(log_file, "Packet loss: %.2f%%\n", packet_loss);
        fprintf(log_file, "Average RTT: %.6f seconds\n", avg_rtt);
    } else {
        printf("No packets received, cannot calculate average RTT\n");
        fprintf(log_file, "\nSummary: No packets received\n");
    }
    
    // Cleanup
    if (payload) free(payload);
    free(buffer);
    close(client_socket);
    fclose(log_file);
    
    return 0;
} 