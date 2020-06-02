#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "parser.h"
#include <sys/time.h>
#include <errno.h>

struct sockaddr_in client_tcp;

int check_time_out(int packet_id_high, int packet_id_low);
int check_compression(int received, int time_low, int time_high);


int set_up_socket(){
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    //int reuse_add = 1;
    int socket_options = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
    if (socket_options < 0 || sock < 0)
    {
        printf("issues with setting up socket. Bye");
        close(sock);
        return EXIT_FAILURE;
    }

    printf("socket created\n");
    return sock;
}

int connect_socket_to_IP(int socket, struct config_info *info){

    struct sockaddr_in ip_address;
    // struct sockaddr_in client_tcp;

    ip_address.sin_family = AF_INET;
    ip_address.sin_addr.s_addr = INADDR_ANY; //ADD ANY IP ADDRESS
    ip_address.sin_port = htons(info->client_port_tcp);

    int bind_check = bind(socket, (struct sockaddr*) &ip_address, sizeof(ip_address));

    if (bind_check < 0)
    {
        printf("Can not bind to port: %d\n", info->client_port_tcp);
    }

    int listen_check = listen(socket, 10);
    if (listen_check < 0)
    {
         printf("Can not listen\n");
    }

    int address_length = sizeof(ip_address);
    int client_length = sizeof(client_tcp);
    int connection = accept(socket, (struct sockaddr*) &ip_address, (socklen_t*) &address_length);

    if (connection < 0)
    {
        printf("Can not establish a connection\n");
    }
    
    int peer = getpeername(connection, (struct sockaddr*) &client_tcp, (socklen_t*) &client_length);
    if (peer < 0)
    {
        printf("Can not set up post-probing info");
        close(connection);
        close(socket);
        return EXIT_FAILURE;
    }
    

    return connection;
}

//CHANGE THIS
int packet_id(char* buffer) {
    int id = 0;
    uint8_t msb, lsb;
    msb = buffer[0];
    lsb = buffer[1];
    id = msb * 256 + lsb;
    if (id < 0 || id > 65536) {
        return -1;
    }
    return id;
}

int set_udp_socket(){

    struct timeval time;
    time.tv_sec = 15;
    time.tv_usec = 0;

    int udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    int udp_options_time_out = (udp_socket, SOL_SOCKET, SO_RCVTIMEO, &time, sizeof(time)); //set up socket options with time out interval
    int udp_options = (udp_socket, SOL_SOCKET, SO_RCVTIMEO, &(int){1}, sizeof(time));

    if (udp_options < 0 || udp_socket < 0 || udp_options_time_out < 0)
    {
        printf("issues with setting up UDP socket. Bye");
        close(udp_socket);
        return EXIT_FAILURE;
    }

    return udp_socket;
}

int run_udp_phase(int udp_socket, struct config_info *info){

        struct sockaddr_in udp_address;
        struct sockaddr_in client_udp;

     int size = info->payload_size + 2;

    char packets[size];

    memset(packets, 0, size * sizeof(char));

    memset(&udp_address, 0, sizeof(udp_address));
    memset(&client_udp, 0, sizeof(client_udp));

    udp_address.sin_family = AF_INET;
    udp_address.sin_addr.s_addr = INADDR_ANY;
    udp_address.sin_port = htons(info->dest_port_udp);

    int udp_bind = bind(udp_socket, (const struct sockaddr *) &udp_address, sizeof(udp_address));

    if (udp_bind < 0)
    {
        printf("Could not bind UDP socket");
        close(udp_socket);
        return EXIT_FAILURE;
    }

    printf("Bound UDP\n");



    //low entropy
    struct  timeval start_low;
    struct timeval end_low;
    struct timeval result_low;

    int time_low;
    int udp_packet_size = 0;
    int packet_id_low = -1;
    int udp_len = sizeof(client_udp);

    

    printf("Getting low packet train\n");

    gettimeofday(&start_low, NULL);

    while (packet_id_low < info->udp_num_packets - 1)
    {
        udp_packet_size = recvfrom(udp_socket, (char *) packets, size, MSG_WAITALL, (struct sockaddr *) &client_udp, (socklen_t *) &udp_len);

        if (udp_packet_size < 0)
        {
            if (errno == EWOULDBLOCK)
            {
                printf("Time out: Low Entropy\n");
                gettimeofday(&end_low, NULL);
                break;
            }
            printf("Can not get Low Entropy data\n");
            close(udp_socket);
            return EXIT_FAILURE;
        }

        //get packet ID number
        packet_id_low = packet_id(packets);

        if (packet_id_low == 0) //first packet
        {
            gettimeofday(&start_low, NULL); //set timer again to the start
        }

        else if (packet_id_low == (info->udp_num_packets - 1))
        {
            gettimeofday(&end_low, NULL);
        }
        printf("Low Entropy pakcet ID: %d\n", packet_id_low);
        memset(packets, 0, size * sizeof(char)); 
    }

    timersub(&end_low, &start_low, &result_low);
    time_low = result_low.tv_usec / 1000;
    printf("Low entropy time: %dms\n", time_low); //CHANGE

    printf("Measurement time: %ld\n", info->time_measurement);

    sleep(info->time_measurement);

    //HIGH ENTROPY PACKET TRAIN
    printf("Getting High Entropy Packet Train\n");


    struct  timeval start_high;
    struct timeval end_high;
    struct timeval result_high;

    int time_high;
    int udp_packet_size_high = 0;
    int packet_id_high = -1;
    udp_len = sizeof(client_udp);

    udp_packet_size = 0; //reset packet size

    memset(packets, 0, size * sizeof(char));
    

    while (packet_id_high < (info->udp_num_packets -1))
    {
       udp_packet_size = recvfrom(udp_socket, (char *) packets, size, MSG_WAITALL, (struct sockaddr *) &client_udp, (socklen_t *) &udp_len);


        if (udp_packet_size < 0)
        {
            if(errno == EWOULDBLOCK){
                printf("Error with high packet train\n");
                gettimeofday(&end_high, NULL);
                 break;  
            }
            printf("Error with high packet train\n");
            close(udp_socket);
            return EXIT_FAILURE;

        }
        
          //get packet ID number
        packet_id_high = packet_id(packets);

        if (packet_id_high == 0) //first packet
        {
            gettimeofday(&start_high, NULL); //set timer again to the start
        }

        else if (packet_id_high == (info->udp_num_packets - 1))
        {
            gettimeofday(&end_high, NULL);
        }
        printf("High Entropy pakcet ID: %d\n", packet_id_high);
        memset(packets, 0, size * sizeof(char)); 
    }

    timersub(&end_high, &start_high, &result_high);
    time_high = result_high.tv_usec / 1000;
    printf("High entropy time: %dms\n", time_high); //CHANGE

    close(udp_socket);
    

    int received = check_time_out(packet_id_high, packet_id_low);
    int detected = check_compression(received, time_low, time_high);


    return detected;
    

}


int check_time_out(int packet_id_high, int packet_id_low){


    int received = 0;

    if (packet_id_low == -1 ||  packet_id_high == -1)
    {
        printf("Insufficient data to determine compression!\n");
        return EXIT_FAILURE;
    }

    return received;
}

int check_compression(int received, int time_low, int time_high){

    int treshold = 100; //100ms 

    int difference; //need to check difference between high and low entropy data
    difference = time_high - time_low; 

    int detected = 0;

    if (received == 0)
    {
        if (treshold < difference)
        {
            printf("Compression detected with a difference of: %d ms\n", difference);
            detected = 1;
            //set up socket
            //set up connection
            //send message

        }else{
            printf("No compression detected. Difference of: %d ms\n", difference);
        }
       
    }

    return detected;    //change to difference once client been set up. then fix send_message

}

int set_post_prob_connection(int socket, struct config_info *info){

    // struct sockaddr_in client_tcp;

    client_tcp.sin_family = AF_INET;
    client_tcp.sin_port = htons(info->client_port_tcp); //set port

    int post_prob_connection = connect(socket, (struct sockaddr*) &client_tcp, sizeof(client_tcp));
    if (post_prob_connection < 0)
    {
        printf("Issue connecting to client in post-prob part\n");
        close(socket);
        return EXIT_FAILURE;        
    }
    return post_prob_connection;
}

int send_message_to_client(int socket, int connection, int difference, int detected){


    //this sends 0 or 1 to client. Client will then print compression detected or not based on 1 or 0 value. 
    //instead send time difference to client, and have client convert this (< 100 no compression)

    char message[5000] = {0};
    memset(message, 0, 5000 * sizeof(char));

    message[0] = detected + '0';
    message[1] = '\0';

    int send_check = send(socket, message, strlen(message), 0);
    if (send_check < 0) 
    {
        printf("Issue sending message post prob\n");
        close(connection);
        close(socket);
        return EXIT_FAILURE;
    }

    close(connection);
    close(socket);

    return 1;
}

size_t write_to_file(int socket, FILE *file){

    size_t bytes_written;

    int bytes_read;
    char buf2[5000] = {0};  
    memset(buf2, 0, 5000 * sizeof(char));
    while ((bytes_read = read(socket, buf2, 5000)) != 0)
    {
        if (bytes_read < 0)
        {
            printf("Server: could not read config file from client\n");
            close(socket);
            return EXIT_FAILURE;
        }

        bytes_written += bytes_read;
        fprintf(file, "%s", buf2);
        printf("Buffer: %s", buf2);
        memset(buf2, 0, 5000 * sizeof(char));
        
    }

    return bytes_written;

}

int read_file(int connection, int socket, struct config_info *info){

    char buf[5000];
    char answer[5000];

    
    int bytes = read(connection, buf, 5000); 
    if (bytes < 0)      
    {
        printf("Can not read\n");
        close(connection);
        close(socket);
        return EXIT_FAILURE;
    }
    printf("Client: %s\n", buf); 

    //check if we are sending the right thing by looking for .ini extension 
    //actual config file may havea  different name, but needs to have .ini format anyways

    printf("checking for config file");
    if (strstr(buf, ".ini") != 0)
    {
        printf("%s\n", buf);
        strcpy(answer, "Server: Server Accepted Config File");
        bytes = send(connection, answer, 100, 0); //send message to client
        if (bytes < 0)
        {
           printf("Server: Was not able to send answer to client\n");
           close(connection);
           close(socket);
           return EXIT_FAILURE;
           
        }

        FILE *file = fopen("config_from_client.ini", "w");
        if (file == NULL)
        {
            printf("Can not open file\n");
            exit(1);
        }
        
        //write to file
        size_t size_written = write_to_file(connection, file);

        if (size_written <= 0)
        {
           return EXIT_FAILURE;
        }
        printf("Wrote %ld bytes\n", size_written);
        fclose(file);

        strcpy(info->file_name, "config_from_client.ini");

        if (parse_config(info))
        {
            printf("Parsing of %s successful\n\n", info->file_name);      
        }
        else{
            printf("parsing failed\n");
            return EXIT_FAILURE;
        }

        printf("config_from_client.ini\n-------------\n");  
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


        strcpy(answer, "Server: Received everything");
        bytes = send(connection, answer, 100, 0); //send message to client
        if (bytes < 0)
        {
           printf("Server: Was not able to send answer to client\n");
           close(connection);
           close(socket);
           return EXIT_FAILURE;
           
        }
        buf[0] = '\0';    
    }
    
    return 0;
    

}

int main(int argc, char *argv[]){

    if (argc < 2)
    {
        printf("Not enough arguments. Please include configuration file\n");
        return EXIT_FAILURE;
    }

    printf("Opening Config File: %s\n", argv[1]);
     printf("Parsing Config File\n");

    struct config_info *info = calloc(1, sizeof(struct config_info));
    strcpy(info->file_name, argv[1]);

    if (parse_config(info))
    {
        printf("Parsing of %s successful\n\n", info->file_name);      
    }
    else{
        printf("parsing failed\n");
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

    //set up some timer to check if a server was ever created?

    int socket = set_up_socket();
    printf("\nRun Client now in a separate terminal window\n");
    int connection = connect_socket_to_IP(socket, info);
    int read = read_file(connection, socket, info);
    if (read == 0)
    {
        close(connection);
       close(socket);
       
    }

    printf("sleeping\n");

    //sleeping for 20s for connection reset
    sleep(20);
    printf("Starting UDP...\n");
    

        // //do the udp stuff
        // int udp_socket = set_up_socket();
        // int detected = run_udp_phase(udp_socket, info);
        // int udp_connection = set_post_prob_connection(udp_socket, info);
        // //set up new socket;

        // int post_prob_socket = set_up_socket();
        // int message = send_message_to_client(udp_socket, udp_connection ,detected, detected);


    return 0;
}