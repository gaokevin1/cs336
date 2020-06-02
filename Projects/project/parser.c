
#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdio.h>
#include <ctype.h>



char *trimNonAlphaNum(char *s){
    int i;
    while(!isalnum(*s)){
        s++;
    }

    for (i = strlen(s)-1; i >= 0; i--)
    {
       if (isalnum(s[i]))
       {
           break;
       }
    }
    s[i + 1] = '\0';

    return s;
}

/**
 * Retrieves the next token from a string.
 *
 * Parameters:
 * - str_ptr: maintains context in the string, i.e., where the next token in the
 *   string will be. If the function returns token N, then str_ptr will be
 *   updated to point to token N+1. To initialize, declare a char * that points
 *   to the string being tokenized. The pointer will be updated after each
 *   successive call to next_token.
 *
 * - delim: the set of characters to use as delimiters
 *
 * Returns: char pointer to the next token in the string.
 */
char *next_token(char **str_ptr, const char *delim)
{
    if (*str_ptr == NULL) {
        return NULL;
    }

    size_t tok_start = strspn(*str_ptr, delim);
    size_t tok_end = strcspn(*str_ptr + tok_start, delim);

    /* Zero length token. We must be finished. */
    if (tok_end  == 0) {
        *str_ptr = NULL;
        return NULL;
    }

    /* Take note of the start of the current token. We'll return it later. */
    char *current_ptr = *str_ptr + tok_start;

    /* Shift pointer forward (to the end of the current token) */
    *str_ptr += tok_start + tok_end;

    if (**str_ptr == '\0') {
        /* If the end of the current token is also the end of the string, we
         * must be at the last token. */
        *str_ptr = NULL;
    } else {
        /* Replace the matching delimiter with a NUL character to terminate the
         * token string. */
        **str_ptr = '\0';

        /* Shift forward one character over the newly-placed NUL so that
         * next_pointer now points at the first character of the next token. */
        (*str_ptr)++;
    }

    return current_ptr;
}

struct config_info* parse_config(struct config_info *parsed_config) {

    FILE* fp = fopen(parsed_config->file_name, "r");
    
    if(fp == NULL) {
        return NULL;
    }
    
    char line_array[256] = { 0 };
    while(fgets(line_array, 256, fp) != NULL) {
        char *line = line_array; 
        // printf("%s",line );

        char *next_tok= line;
        char *cur_tok;

        //get values
        if (strstr(line, "ServerIP") != 0)
        {
            next_token(&next_tok, "=");
            cur_tok = next_token(&next_tok, "=");
            // printf("%s\n", cur_tok);
            cur_tok = trimNonAlphaNum(cur_tok);
            parsed_config->server_ip = calloc(strlen(cur_tok) + 1, sizeof(char));
            strcpy(parsed_config->server_ip, cur_tok);
        }
        else if (strstr(line, "SourceUDP") != 0)
        {
            next_token(&next_tok, "=");
            cur_tok = next_token(&next_tok, "=");
            parsed_config->src_port_udp = (unsigned short int)atoi(cur_tok);
        }
        else if (strstr(line, "DestinationUDP") != 0)
        {
            next_token(&next_tok, "=");
            cur_tok = next_token(&next_tok, "=");
            parsed_config->dest_port_udp = (unsigned short int)atoi(cur_tok);
        }
          else if (strstr(line, "HeadDestinationTCP") != 0)
        {
            next_token(&next_tok, "=");
            cur_tok = next_token(&next_tok, "=");
            parsed_config->head_port_tcp = (unsigned short int)atoi(cur_tok);
        }
           else if (strstr(line, "TailDestinationTCP") != 0)
        {
            next_token(&next_tok, "=");
            cur_tok = next_token(&next_tok, "=");
             parsed_config->tail_port_tcp = (unsigned short int)atoi(cur_tok);
        }
           else if (strstr(line, "PortNumberTCP") != 0)
        {
            next_token(&next_tok, "=");
            cur_tok = next_token(&next_tok, "=");
            parsed_config->client_port_tcp = (unsigned short int)atoi(cur_tok);
        }
             else if (strstr(line, "PayloadSizeUDP") != 0)
        {
            next_token(&next_tok, "=");
            cur_tok = next_token(&next_tok, "=");
            parsed_config->payload_size = (unsigned short int)atoi(cur_tok);
        }
            else if (strstr(line, "InterMeasurementTime") != 0)
        {
            next_token(&next_tok, "=");
            cur_tok = next_token(&next_tok, "=");
            parsed_config->time_measurement = (time_t)atoi(cur_tok);
        }
            else if (strstr(line, "NumberPackets") != 0)
        {
            next_token(&next_tok, "=");
            cur_tok = next_token(&next_tok, "=");
            parsed_config->udp_num_packets = (unsigned short int)atoi(cur_tok);
        }
            else if (strstr(line, "TimeToLiveUDP") != 0)
        {
            next_token(&next_tok, "=");
            cur_tok = next_token(&next_tok, "=");
            parsed_config->udp_packet_ttl = (unsigned short int)atoi(cur_tok);
        }
    }

    return parsed_config;

}