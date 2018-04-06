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
#include <signal.h>
#include <sys/wait.h>

#include <socketlib.h>
#include <audioserver.h>
#include <smplutils.h>

unsigned int process_fd[MAX_GUESTS];
struct wav_params params;

int main(){
  //signal(SIGPIPE, SIG_IGN);
  signal(SIGCHLD, close_fd);
  recv_req(); 
}

void close_fd(int sig){
  pid_t pid;
  int status;
  while ((pid = waitpid(-1, &status, WNOHANG)) != -1){
    int i = 0;
    while ((i < MAX_GUESTS) && (process_fd[i] != pid))
      i++;
    printf("closed fd : %d, pid : %d\n", i, pid);
    close(i);
  }
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

    if(recv_packet(&req, sizeof(req), &client) == 1)
      printf("could not recv packet\n");

    //printf("received request for packet %d with token %d \n",req.req_n, req.token);

    if(req.token == 0){
      // client connection
      printf("%s is connected \n",inet_ntoa(client.addr.sin_addr));


      // make a pipe to the thread treating the client
      if(pipe(fds) < 0){
        perror("Could not create pipe "); exit(1);
      }

      // make the thread treating the client
      pid = fork();
      if (pid < 0) { perror("fork() error "); exit(1); }
      if (pid == 0) {
        // child process
        // give it the client's address so it can send packets to him

        close(fds[1]);
        treat_req(fds, &client, &req);
        return 0;

      }
      else{
        //parent process
       	 
	process_fd[fds[1]] = pid;
        close(fds[0]);
        ips_tokens[fds[1]] = client.addr.sin_addr.s_addr;
    
      }

    }
    else if( fcntl(req.token, F_GETFD) != -1 ){
      // if token is a valid and open filedescriptor 
      // redirect req to pipe treating that client
      
      if( ips_tokens[req.token] == client.addr.sin_addr.s_addr ){
      
        if(write(req.token, &req, sizeof(req)) < 0){
          perror("could not write to file descriptor");
        }
      
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


int treat_req(int* fds, struct dest_infos* client, struct request* init_req){
  struct audio_packet packet;
  int rea;
  //TODO : chemin absolu
  char prefixe[134] = "audio/";
  strcat(prefixe,init_req->filename);
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

    
    params.channels = info[2];
    params.sample_rate = info[0];
    params.sample_size = info[1];

    //mvprintw(0,1,"%d");
    if(init_req->mono == 1){
      info[2] = 1;
    } 

    // assigning a token to the client
    info[3] = fds[1];


    send_packet(info, sizeof(info), client);

    packet.header = 0;
    uint8_t tmp[BUF_SIZE];
    int nsmpl = BUF_SIZE / (params.sample_size/8);
    int16_t * audio16;
    int16_t * tmp16;
    do{

      if(read(fds[0], &req, sizeof(req)) < 0){
        perror("Could not read file descriptor");
      }

      // received end of connection request
      if(req.req_n == -1){
        packet.header = -1;
      }
      else if(packet.header + 1 == req.req_n){
        packet.header++;
        if(init_req->mono && (params.channels == 2) ){
          // asked for mono and file is stereo
          // merge both channels into one
          for(int b = 0; b < 2; b++){
            rea = read(fdr, tmp, BUF_SIZE);
            if (rea < 0) {
              perror("Could not read wav file");
              exit(1);
            }
            if(params.sample_size == 16){
              tmp16 = (int16_t *) tmp;
              audio16 = (int16_t *) packet.audio;
            }
            for(int s = 0; s < nsmpl/2; s++){        
              switch(params.sample_size){
                case 16:
                  audio16[s+(nsmpl/2)*b] = tmp16[s*2];//*0.5f + tmp16[s*2+1]*0.5f;
                  break;
                case 8:
                  packet.audio[s+(nsmpl/2)*b] = tmp[s*2]*0.5f + tmp[s*2+1]*0.5f;
              }
            }
          }
        }
        else{
          // did not ask for mono or already is
          rea = read(fdr, packet.audio, BUF_SIZE);
          if (rea < 0) {
            perror("Could not read wav file");
            exit(1);
          }
        }
      }

      if(rea == 0){
        packet.header = -1;
      }
      //printf("sent packet %d to token %d\n",packet.header,fdr);
      send_packet(&packet, sizeof(packet), client);

    } while (rea != 0 && packet.header != -1);

    packet.header = -1;

    printf("sent EOF %d to token %d\n",packet.header,fdr);
    send_packet(&packet, sizeof(packet), client);
    close(fdr);
    close(fds[0]);

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
