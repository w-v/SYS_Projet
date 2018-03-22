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
    perror("Could not send packet");
    return 1;
  }
  return 0;

}

int send_until_ack( void * packet, short unsigned int size, struct dest_infos * infos ){

  fd_set rfds;
  struct timeval tv;// = {5};
  int retval = 0;
  char msg[128];

  tv.tv_usec = 0;
  FD_ZERO(&rfds);

  while(retval == 0 && !(FD_ISSET(infos->fd, &rfds)) ){

    FD_ZERO(&rfds);
    FD_SET(infos->fd, &rfds);
    tv.tv_sec = 2;
    
    send_packet(packet, size, infos);

    retval = select(infos->fd+1, &rfds, NULL, NULL, &tv);

    if (retval == -1){
      perror("select() failed");
      return 1;
    }
    else if ( (retval == 1) && FD_ISSET(infos->fd, &rfds))
      recv_packet(msg, sizeof(msg), infos);

    //printf("%d \n",FD_ISSET(infos->fd, &rfds));

  }
  return 0;

} 

