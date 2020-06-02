/**
 * @file udp_payload.c
 * @author Graham Hendry & Ghufran latif (gphendry@dons.usfca.edu, glatif@dons.usfca.edu)
 * @brief Library file for functions that read/modify UDP packet payload in binary form
 * @version 0.1
 * @date 2020-03-28
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/random.h>

#include "udp_payload.h"
/**
 * @brief Set the packet id
 * 
 * @param buffer UDP packet payload
 * @param id integer ID to be set as first two bytes of payload
 */
void set_packet_id(char* buffer, int id) {
    buffer[0] = (uint8_t) (id / 256);
    buffer[1] = (uint8_t) (id % 256);
}

/**
 * @brief Get the packet id from the buffer
 * 
 * @param buffer UDP packet payload buffer
 * @return int packed ID in integer form, -1 if ID number is invalid
 */
int get_packet_id(char* buffer) {
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

/**
 * @brief Sets the high-entropy packet payloads with bits from /dev/random
 * 
 * @param buffer char array for UDP packet payload
 * @param len payload buffer size
 * @return int number of bytes copied into buffer if successful, -1 if error
 */
int set_high_entropy(char* buffer, int len) {
    int num_rand_bytes = getrandom(buffer, len, 0); // set last arg to GRND_RANDOM to read from /dev/random
    if (num_rand_bytes < 0) {
        perror("Set high-entropy payload");
        return -1;
    }
    return num_rand_bytes;
}