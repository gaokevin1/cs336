#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/random.h>
#include <time.h>
#include <errno.h>

#include "parser.h"

int set_socket();
int set_udp_socket();
int set_post_socket();

int connect_to_socket(int socket, struct config_info *info);
size_t send_file(int socket, int connection, struct config_info *info);

int get_udp_id(char* buf);
void set_udp_id(char* buf, int id);
int get_random_payload(char* buf, int size);

int init_udp_transfer(struct config_info *info, int sockfd);
int run_udp_phase(struct config_info *info, struct sockaddr_in server_addr, 
                    char* buf, int buf_size, int udp_sock, int connection);
int run_post_phase(struct config_info *info);

int msleep(long msec);

/* Return new general socket with specified options */
int set_socket() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        printf("Could not create socket.\n\n");
        return EXIT_FAILURE;
    }

    int socket_options = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
    if (socket_options < 0) {
        printf("There was a problem with the socket options.\n\n");
        close(sock);
        return EXIT_FAILURE;
    }

    return sock;
}

/* Return new UDP socket with specified options */
int set_udp_socket() {
    int udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_sock < 0) {
        printf("Could not create socket.\n\n");
        return EXIT_FAILURE;
    }

    if (setsockopt(udp_sock, IPPROTO_IP, IP_MTU_DISCOVER, &(int){IP_PMTUDISC_DO}, sizeof(IP_PMTUDISC_DO)) < 0) {
        printf("Set UDP DF flag.\n\n");
        return EXIT_FAILURE;
    }

    if (setsockopt(udp_sock, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) {
        printf("Set client UDP reuse option.\n\n");
        close(udp_sock);
        return EXIT_FAILURE;
    } 
    return udp_sock;
}

/* Return new post-probing socket with specified options */
int set_post_socket() {
    int post_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (post_sock < 0) {
        printf("Could not create post-probing socket.\n\n");
        return EXIT_FAILURE;
    }

    if (setsockopt(post_sock, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) {
        printf("Could not use post-probing socket options.\n\n");
        close(post_sock);
        return EXIT_FAILURE;
    }
    return post_sock;
}

/* Function to connect to socket */
int connect_to_socket(int socket, struct config_info *info) {
    struct sockaddr_in server;

    memset(&server, 0, sizeof(server));
    server.sin_port = htons(info->client_port_tcp);
    server.sin_family = AF_INET;

    int converted_ip = inet_pton(AF_INET, info->server_ip, &server.sin_addr);
    if (converted_ip < 0) {
        printf("Could not convert the IP address.\n\n"); //see if we can comebine error checking in to one if statement
        close(socket);
        return EXIT_FAILURE;
    }

    int connection = connect(socket, (const struct sockaddr *)&server, sizeof(server));
    if (connection < 0) {
        printf("Could not create a connection.\n\n");
        close(socket);
        return EXIT_FAILURE;
    }
    return connection; 
}

/* Send file to server over socket */
size_t send_file(int socket, int connection, struct config_info *info) {
    char buf[2048] = {0};
    strcpy(buf, info->file_name);

    int send_fn = send(socket, buf, 2048, 0);
    if (send_fn == -1) {
        printf("Could not send file name.\n");
        printf("Could not set up socket.\n\n");
        close(connection);
        close(socket);
        return EXIT_FAILURE;
    }

    FILE *file = fopen(info->file_name, "r");
    if (file == NULL) {
        printf("Could not open file.\n\n");
        fclose(file);    
        close(connection);
        close(socket);
        return EXIT_FAILURE;
    }

    size_t bytes_sent = 0;

    // Send file line by line
    char line[5000]= {0};
    memset(line, 0, 5000 * sizeof(char));
    
    while (fgets(line, 5000, file) != NULL)
    {   
        bytes_sent += strlen(line);
        // printf("Line (sent to server): %s\n", line);
        msleep(1);
        write(socket, line, sizeof(line));
        memset(line, 0, 5000 * sizeof(char));
    }

    // Check in with socket
    char buf2[5000] = {0};
    int server_msg = recv(socket, buf2, 4999, 0);

    if (server_msg == -1) {
        printf("Could not receive message from server.\n\n");
        fclose(file);
        close(connection);
        close(socket);
        return EXIT_FAILURE;
    } else {
        printf("Received from %s", buf2);
    }

    fclose(file);
    close(connection);
    close(socket);

    if (bytes_sent <= 0) {
        printf("Nothing was sent. Quitting...\n");
        return EXIT_FAILURE;
    }

    return bytes_sent; 
}

/* Set UDP packet ID */
void set_udp_id(char* buf, int id) {
    // Sets the first two bytes in the train as the packet ID
    buf[0] = (uint8_t) (id / 256); // most significant
    buf[1] = (uint8_t) (id % 256); // least significant
}

/* Get UDP packet ID */
int get_udp_id(char* buf) {
    uint8_t least, most;
    least = buf[1];
    most = buf[0];

    int packet_id = least + most * 256;
    // Check to see if packet ID is valid
    if (packet_id < 0) {
        return -1;
    }
    return packet_id;
}

/* Fills entropy packets with random data for payload */ 
int get_random_payload(char* buf, int size) {
    int buf_size = getrandom(buf, size, 0);
    if (buf_size < 0) {
        printf("Could not properly fill buffer.\n\n");
        return -1;
    }
    return buf_size;
}

/* Starts up sending the UDP packet trains */
int init_udp_transfer(struct config_info *info, int sockfd) {
    // Sending UDP packet train
    struct sockaddr_in server_addr, client_addr;
    // Adds two bytes (16 bits) to total buffer size to store packet ID in binary form
    int buf_size = info->payload_size + 2;
    printf("Size of UDP packet train: %dB\n", buf_size);

    char buf[buf_size];
    memset(buf, 0, buf_size * sizeof(char));

    // Create udp socket
    int udp_sock = set_udp_socket();
    
    // Set server port info
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(info->dest_port_udp);

    // Set client port info
    memset(&client_addr, 0, sizeof(client_addr));
    client_addr.sin_addr.s_addr = INADDR_ANY;
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(info->src_port_udp);

    // Socket error checking
    int b = bind(udp_sock, (struct sockaddr *)&client_addr, sizeof(client_addr));
    if (b < 0) {
        printf("Could not bind to port.\n\n");
        close(udp_sock);
        return EXIT_FAILURE;
    }
    
    int c = inet_pton(AF_INET, info->server_ip, &server_addr.sin_addr);
    if (c < 0)
    {
        printf("Could not convert the IP address.\n\n");
        close(udp_sock);
        return EXIT_FAILURE;
    }

    int connection = connect(udp_sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (connection < 0) {
        printf("Could not connect via UDP.\n\n");
        close(sockfd);
        return EXIT_FAILURE;
    }
    // Run the low/high entropy data tests
    run_udp_phase(info, server_addr, buf, buf_size, udp_sock, connection);

    return 1;
}

/* Runs through low/high entropy packet train tests */ 
int run_udp_phase(struct config_info *info, struct sockaddr_in server_addr,
                    char* buf, int buf_size, int udp_sock, int connection) {
    // Send low-entropy packet train
    printf("Sending low-entropy train...\n\n");
    int send_size = 0;
    for (int i = 0; i < info->udp_num_packets; i++) {
        // Set packet ID for UDP payload
        set_udp_id(buf, i);
        printf("Sent low-entropy packet: %d\n", get_udp_id(buf));
        
        send_size = sendto(udp_sock, buf, sizeof(buf), MSG_CONFIRM, (const struct sockaddr *)&server_addr, (socklen_t)sizeof(server_addr));
        // Make sure that a valid buffer was sent
        if (send_size < 0) {
            printf("Could not send packet train.\n\n");
            close(connection);
            close(udp_sock);
            return EXIT_FAILURE;
        }
        memset(buf, 0, (buf_size * sizeof(char)) + 2);  
    }
    printf("Low-entropy packet train was successfully sent.\n\n");

    printf("Waiting %ld secs for IMT...\n", info->time_measurement);
    sleep(info->time_measurement);

    // Reset send_size variable for high-entropy tests
    send_size = 0;

    // Sends high-entropy packet train
    printf("Sending high-entropy train...\n\n");
    memset(buf, 0, buf_size * sizeof(char)); 

    for (int i = 0; i < info->udp_num_packets; i++) {
        // Set packet ID for UDP payload
        set_udp_id(buf, i);
        printf("Sent high-entropy packet: %d\n", get_udp_id(buf));
        
        // Gets random data to store in payload for high-entropy train
        get_random_payload(buf + 2, info->payload_size * sizeof(char));
        // Adds null-byte at the end of the char array
        buf[buf_size - 1] = '\0';
        
        send_size = sendto(udp_sock, buf, sizeof(buf), MSG_CONFIRM, (const struct sockaddr *)&server_addr, (socklen_t)sizeof(server_addr));
        // Make sure that a valid buffer was sent
        if (send_size < 0) {
            printf("Could not send packet train.\n\n");
            close(connection);
            close(udp_sock);
            return EXIT_FAILURE;
        }
        memset(buf, 0, buf_size * sizeof(char)); 
    }
    printf("High-entropy packet train was successfully sent.\n\n");
    close(connection);
    close(udp_sock);

    // Run post-probing phase of testing
    run_post_phase(info);

    return 1;
}

/* Runs through post-probing phase of testing */ 
int run_post_phase(struct config_info *info) {
    struct sockaddr_in final;
    memset(&final, 0, sizeof(final));
    int size = sizeof(final);

    // Create buffer for final results
    char buf[2048];
    memset(buf, 0, sizeof(buf) * sizeof(char));

    // Create post socket
    int post_sock = set_post_socket();

    // Sets port info
    final.sin_addr.s_addr = INADDR_ANY;
    final.sin_family = AF_INET;
    final.sin_port = htons(info->client_port_tcp);

    // Socket error checking
    int b = bind(post_sock, (struct sockaddr *)&final, (socklen_t)size);
    if (b < 0) {
        printf("Could not bind post-probing socket to port.\n\n");
        close(post_sock);
        return EXIT_FAILURE;
    }

    int l = listen(post_sock, 10);
    if (l < 0) {
        printf("Could not hear post-probing socket.\n\n");
        close(post_sock);
        return EXIT_FAILURE;
    }

    int post_connection = accept(post_sock, (struct sockaddr *)&final, (socklen_t *)&size);
    if (post_connection < 0) {
        printf("Could not establish post-probing socket connection.\n\n");
        return EXIT_FAILURE;
    }

    // Reads post-probing final sent from server
    int returned_buf = read(post_connection, buf, 2048);
    if (returned_buf < 0) {
        printf("Could not receive post-probing buffer.\n\n");
        close(post_connection);
        close(post_sock);
        return EXIT_FAILURE;
    }
    printf("Received from server: %s\n", buf);

    int difference = atoi(buf);
    // Prints final based on first character in post-probing buffer
    if (difference <= 100) {
        printf("Could not detect any compression.\n\n");
    } else {
        printf("Detected Compression!\n\n");
    }

    close(post_connection);
    close(post_sock);

    return 1;
}

/* Sleep in milliseconds (from Stack Overflow) */
int msleep(long msec) {
    struct timespec ts;
    int res;

    if (msec < 0) {
        errno = EINVAL;
        return -1;
    }

    ts.tv_sec = msec / 1000;
    ts.tv_nsec = (msec % 1000) * 1000000;

    do {
        res = nanosleep(&ts, &ts);
    } while (res && errno == EINTR);

    return res;
}

/* Main function */
int main(int argc, char *argv[]) {
    // If no config file provided
    if (argc < 2) {
        printf("You have not included a config file for the client.\n");
        return EXIT_FAILURE;
    }
    
    printf("Opening Config File: %s\n", argv[1]);
    printf("Parsing Config File\n");

    struct config_info *info = calloc(1, sizeof(struct config_info));
    strcpy(info->file_name, argv[1]);

    // Parse config.ini for necessary server info
    if (parse_config(info)) {
        printf("Parsing of %s was successful.\n\n", info->file_name);      
    } else {
        printf("Parsing failed.\n");
        return EXIT_FAILURE;
    }

    printf("Server IP address: %s\n", info->server_ip);
    printf("Source Port Number for UDP: %d\n", info->src_port_udp);
    printf("Destination Port Number for UDP: %d\n", info->dest_port_udp);
    printf("TCP Head: %d\n", info->head_port_tcp);
    printf("TCP Tail: %d\n", info->tail_port_tcp);
    printf("Port number for TCP: %d\n", info->client_port_tcp);
    printf("Payload size: %d\n", info->payload_size);
    printf("Inter-Measurement Time: %ld\n", info->time_measurement);
    printf("Number of UDP packets in Train: %d\n", info->udp_num_packets);
    printf("TTL for UDP: %d\n", info->udp_packet_ttl);

    // Setup socket
    int sock = set_socket();
    // Connect to new socket
    int connection = connect_to_socket(sock, info);
    // Test socket by sending config file to server
    send_file(sock, connection, info);

    // Have the server wait for UDP
    printf("\n\nWaiting for the server to reset...\n");
    sleep(25);
    printf("Starting low/high entropy UDP tests...\n\n");

    // Start UDP packet train transfers
    init_udp_transfer(info, sock);

    return 0;
}