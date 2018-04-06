#include <socketlib.h>


int send_packet( void * packet, short unsigned int size ,struct dest_infos * infos) {

  socklen_t len = sendto(infos->fd, packet, size, 0, (struct sockaddr*) &(infos->addr), infos->flen);
  if(len < 0){
    perror("Could not send packet");
    return 1;
  }
  return 0;

}

int recv_packet( void * packet, short unsigned int size, struct dest_infos * infos) {

  socklen_t len = recvfrom(infos->fd, packet, size, 0, (struct sockaddr*) &(infos->addr), &(infos->flen));
  if(len < 0){
    perror("Could not recv packet");
    return 1;
  }
  return 0;

}

