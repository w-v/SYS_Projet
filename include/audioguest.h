#include <sys/types.h>
#include <sys/socket.h>
#include <audio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <dirent.h> 
#include <string.h>

#include <netdb.h>

#include <socketlib.h>

int socket_guest_init( struct dest_infos* server, char* hostname);

int req_until_ack( struct request* req, short unsigned int rsize, struct audio_packet* packet, short unsigned int size, struct dest_infos* infos );

char* resolv_hostname(const char *hostname); 

