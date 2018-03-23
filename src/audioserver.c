#include <audioserver.h>

int main(){
    recv_req(); 
}

int recv_req(){

  int pid;
  int fds[2];
  struct dest_infos client;
  struct request req;
  int ips_tokens[256];             

  // tokens identify clients, for convenience,
  // they're equal to the write fd of the pipe
  // between this thread and the thread treating 
  // the client
  
  // It does create a security problem, as anyone can try to send a token
  // inferior to his own a potentially get somebody else's packet
  // That's why ips_tokens keeps the link between ips and tokens

  socket_server_init( &client );



  while(1){


    recv_packet(&req, sizeof(req), &client);

    //printf("received request for packet %d with token %d \n",req.req_n, req.token);

    if(req.token == 0){
      // client connection
      printf("%s is connected",inet_ntoa(client.addr.sin_addr));
      

      // make a pipe to the thread treating the client
      if(pipe(fds) < 0){
        perror("Could not create pipe "); exit(1);
      }
      
      printf( "0: %d, 1: %d",fds[0],fds[1]);

      // make the thread treating the client
      pid = fork();
      if (pid < 0) { perror("fork() error "); exit(1); }
      if (pid == 0) {
        // child process
        // give it the client's address so it can send packets to him

        close(fds[1]);
        treat_req(fds, &client, req.filename);
        exit(0);

      }
      else{
        //parent process
        
        close(fds[0]);
        ips_tokens[fds[1]] = client.addr.sin_addr.s_addr;
    
      }

    }
    else if( fcntl(req.token, F_GETFD) != -1 ){
      // if token is a valid and open filedescriptor 
      // redirect req to pipe treating that client
      
      if( ips_tokens[req.token] == client.addr.sin_addr.s_addr ){
      
        write(req.token, &req, sizeof(req));
      
      }
      else{

        printf("received wrong token from ip %s", inet_ntoa(client.addr.sin_addr));

      }


    }
    else {

      printf("token : %d was not recognized",req.token);

    }


  }

}


int treat_req(int* fds, struct dest_infos* client, char * filename){
  struct audio_packet packet;
  int rea;
  //TODO : chemin absolu
  char prefixe[134] = "audio/";
  strcat(prefixe,filename);

  if(access(prefixe, F_OK) == -1){
    packet.header = -1;
    send_packet(&packet, sizeof(packet), client);
  }
  else{
    char * filename = prefixe;
    int info[4];
    struct request req;

    int fdr = aud_readinit( filename,&(info[0]),&(info[1]),&(info[2]));
    if(fdr < 0){
      perror("Could not get speaker's file descriptor");
    }       

    printf("%d %d %d",info[0], info[1], info[2]);

    // assigning a token to the client
    info[3] = fds[1];


    send_packet(info, sizeof(info), client);

    packet.header = 0;

    do{

      read(fds[0], &req, sizeof(req));

      if(packet.header + 1 == req.req_n){
        packet.header++;
        rea = read(fdr, packet.audio, 1024);
        if (rea < 0) {
          perror("Could not read wav file");
          exit(1);
        }
      }

      send_packet(&packet, sizeof(packet), client);

    } while (rea != 0);

    packet.header = -1;

    send_packet(&packet, sizeof(packet), client);


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
