#include <audioserver.h>

int main(){
  int rea;
  char msg[128];
  char packet[1024];
  struct dest_infos client;

  //TODO : chemin absolu
  char prefixe[134] = "audio/";

  socket_server_init( &client );

  recv_packet(msg, sizeof(msg), &client);
  strcat(prefixe,msg);

  if(access(prefixe, F_OK) == -1){
    *( (int*) packet ) = -1;
    send_packet(packet, sizeof(packet), &client);
  }
  else{
    char * filename = prefixe;
    int info[3];

    int fdr = aud_readinit( filename,&(info[0]),&(info[1]),&(info[2]));
    if(fdr < 0){
      perror("Could not get speaker's file descriptor");
    }       

    printf("%d %d %d",info[0], info[1], info[2]);

    //TODO: un ptit timeout
    send_packet(info, sizeof(info), &client);
    recv_packet(msg, sizeof(msg), &client);

    do{
      rea = read(fdr, packet, 1024);
      if (rea < 0) {
        perror("Could not read wav file");
        exit(1);
      }

      send_packet(packet, sizeof(packet), &client);

      recv_packet(msg, sizeof(msg), &client);

    } while (1);

  }
  return 0;
}

int socket_server_init( struct dest_infos* client ){
	
	struct sockaddr_in addr;
	
	client->flen = sizeof(struct sockaddr_in);

	client->fd = socket(AF_INET,SOCK_DGRAM,0);
	if (client->fd < 0) {
		perror("Could not create socket");
		return 1;
	}

	addr.sin_family = AF_INET;
	addr.sin_port = htons(47777);
	addr.sin_addr.s_addr = htons(INADDR_ANY);

	int err = bind(client->fd, (struct sockaddr *) &addr, client->flen);
	if (err < 0){
		perror("Could not bind socket");
		return 1;
	}
	return 0;
}
