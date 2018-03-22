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
#include <sys/time.h>

#define PACKET_SIZE 1024

struct dest_infos {

  int fd;
  struct sockaddr_in addr;
  socklen_t flen;

};

struct audio_packet {

  int header; 
  char audio[PACKET_SIZE];

};

int send_packet( void * packet, short unsigned int size, struct dest_infos * infos);

int recv_packet( void * packet, short unsigned int size, struct dest_infos * infos);
