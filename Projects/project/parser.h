#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <arpa/inet.h>        
#include <sys/ioctl.h>        
#include <bits/ioctls.h>      
#include <net/if.h>           
#include <linux/if_ether.h>   
#include <linux/if_packet.h>  
#include <net/ethernet.h>

#ifndef _PARSER_H_
#define _PARSER_H_

#include <sys/types.h>
//#include "udp_client.h"

struct config_info {
    char file_name[1024];
    char *server_ip;
    int src_port_udp;
    int dest_port_udp;
    int head_port_tcp;
    int tail_port_tcp;
    int client_port_tcp;
    int payload_size;
    time_t time_measurement;
    int udp_num_packets;
    int udp_packet_ttl;
};

struct config_info* parse_config(struct config_info *parsed_config);

#endif