#ifndef UDP_PAYLOAD_H
#define UDP_PAYLOAD_H

void set_packet_id(char * buffer, int id);
int get_packet_id(char* buffer);
int set_high_entropy(char* buffer, int len);

#endif