#include <audioguest.h>
int main(int argc, char* argv[]){

  if(argc != 3){
    printf("\nUsage : \n\n audioguest server_hostname file_name \n\n");
    exit(1);
  }

  int wri;
  int req_n = 1;

  char msg[128];
  struct audio_packet packet;
  packet.header = 0;
  struct dest_infos server;

  socket_guest_init( &server, argv[1]);

  if (!send_packet(argv[2], sizeof(argv[2]), &server)){ 

    int info[3];
    recv_packet(info, sizeof(info), &server);

    if(info[0] == -1){            // dans le cas o� le filename n'est pas trouvé ar
      perror("Server says the file was not found"); //le serveur, celui ci renvoie
      exit(1);                                     // -1 dans le champs reservéau
    }                                             // channel.

    int fdw = aud_writeinit(info[0],info[1],info[2]);
    if(fdw < 0){
      perror("Could not get speaker's file descriptor");
    }       

    do{

      req_until_ack(req_n, &packet, sizeof(packet), &server);
      req_n += 1;

      wri = write(fdw, packet.audio , 1024);
      if (wri < 0) {
        perror("Could not write to speaker");
        exit(1);
      }

      usleep(1000);
      //printf("%d \n", packet.header);

    }while(packet.header != -1);
  }

  return 0;
}

int socket_guest_init( struct dest_infos* server, char* hostname){

  server->fd = socket(AF_INET,SOCK_DGRAM,0);

  server->flen = sizeof(struct sockaddr_in);

  if (server->fd < 0) {

    perror("Could not create socket to server");

  }

  (server->addr).sin_family = AF_INET;
  (server->addr).sin_port = htons(47777);
  (server->addr).sin_addr.s_addr = inet_addr(resolv_hostname(hostname));

  return 0;
}


int req_until_ack( int req_n, struct audio_packet* packet, short unsigned int size, struct dest_infos* infos ){

  fd_set rfds;
  struct timeval tv;// = {5};
  int retval = 0;

  tv.tv_usec = 0;
  FD_ZERO(&rfds);

  while(retval == 0 && !(FD_ISSET(infos->fd, &rfds)) && (packet->header != req_n)){

    FD_ZERO(&rfds);
    FD_SET(infos->fd, &rfds);
    tv.tv_sec = 2;

    send_packet(&req_n, sizeof(req_n), infos);

    retval = select(infos->fd+1, &rfds, NULL, NULL, &tv);

    if (retval == -1){
      perror("select() failed");
      return 1;
    }
    else if ( (retval == 1) && FD_ISSET(infos->fd, &rfds))
      recv_packet(packet, size, infos);

  }
  return 0;

}

char* resolv_hostname(const char *hostname) {
  struct hostent *resolv;
  struct in_addr *addr;
  resolv = gethostbyname(hostname);
  if (resolv==NULL) {
    printf("host %s could not be resolved", hostname);
    exit(1);
  }
  else {
    addr = (struct in_addr*) resolv->h_addr_list[0];
    printf("The IP address of %s is %s\n",hostname,inet_ntoa(*addr));
    return inet_ntoa(*addr);
  }
}
