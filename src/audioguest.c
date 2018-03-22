#include <audioguest.h>

int main(){
  int wri;

  char msg[128] = "ring.wav";
  struct audio_packet packet;
  struct dest_infos server;

  socket_guest_init( &server );

  if (!send_packet(msg, sizeof(msg), &server)){ 

    int info[3];
    recv_packet(info, sizeof(info), &server);

    if(info[0] == -1){            // dans le cas o� le filename n'est pas trouvé ar
      perror("Server says the file was not found"); //le serveur, celui ci renvoie
      exit(1);                                     // -1 dans le champs reservéau
    }                                             // channel.

    int ok = 1;

    int fdw = aud_writeinit(info[0],info[1],info[2]);
    if(fdw < 0){
      perror("Could not get speaker's file descriptor");
    }       

    do{
      
      recv_packet(packet, sizeof(packet), &server);

      wri = write(fdw, packet.audio , 1024);
      if (wri < 0) {
        perror("Could not write to speaker");
        exit(1);
      }

      send_packet(&ok, sizeof(int), &server);

      usleep(1000);

    }while(packet.header == 0);
  }

  return 0;
}

int socket_guest_init( struct dest_infos* server ){

  server->fd = socket(AF_INET,SOCK_DGRAM,0);

  server->flen = sizeof(struct sockaddr_in);

  if (server->fd < 0) {

     perror("Could not create socket to server");

  }

  (server->addr).sin_family = AF_INET;
  (server->addr).sin_port = htons(47777);
  (server->addr).sin_addr.s_addr = inet_addr("127.0.0.1");

  return 0;
}
