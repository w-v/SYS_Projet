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



struct dest_infos {

  int fd;
  struct sockaddr_in addr;
  socklen_t flen;

}

int send_packet( void * packet, struct dest_infos * infos) {

    len = sendto(infos->fd, packet, sizeof(packet), 0, (struct sockaddr*) &(infos->addr), infos->flen);
    if(len < 0){
      perror("Could not send packet");
      return 1;
    }
    return 0

}

int recv_packet( void * packet, struct dest_infos * infos) {

    len = recvfrom(infos->fd, packet, sizeof(packet), 0, (struct sockaddr*) &(infos->addr), &(infos->flen));
    if(len < 0){
      perror("Could not send packet");
      return 1;
    }
    return 0

}

int socket_init( struct dest_infos* client ){
	
	struct sockaddr_in addr;
	
	client.flen = sizeof(struct sockaddr_in);

	client.fd = socket(AF_INET,SOCK_DGRAM,0);
	if (client.fd < 0) {
		perror("Could not create socket");
		return 1;
	}

	addr.sin_family = AF_INET;
	addr.sin_port = htons(47777);
	addr.sin_addr.s_addr = htons(INADDR_ANY);

	err = bind(client.fd, (struct sockaddr *) &addr, client.flen);
	if (err < 0){
		perror("Could not bind socket");
		return 1;
	}
	return 0;
}

int 


int main(){
  int err, rea;
  char msg[128];
  char packet[1024];
  socklen_t len;
  struct dest_infos client;

  //TODO : chemin absolu
  const char prefixe[134] = "audio/";

  socket_init( &client, &addr );

  recv_packet(msg, &client);
  strcat(prefixe,msg);

  if(access(prefixe, F_OK) == -1){
    *( (int*) packet ) = -1;
    send_packet(packet, &client);
  }
  else{
    char * filename = prefixe;
    int info[3];

    int fdr = aud_readinit( filename,&(info[0]),&(info[1]),&(info[2]));

    printf("%d %d %d",info[0], info[1], info[2]);

    //TODO: un ptit timeout
    send_packet(info, &client);
    recv_packet(msg, &client);

    do{
      rea = read(fdr, packet, 1024);
      if (rea < 0) {
        perror("Could not read wav file");
        exit(1);
      }

      send_packet(packet, &client);

      recv_packet(msg, &client);

    } while (1);

  }
  return 0;
}
