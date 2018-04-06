#include <socketlib.h>

int socket_guest_init( struct dest_infos* server, char* hostname);

int req_until_ack( struct request* req, short unsigned int rsize, struct audio_packet* packet, short unsigned int size, struct dest_infos* infos );

char* resolv_hostname(const char *hostname); 
