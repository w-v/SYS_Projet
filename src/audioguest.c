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
#include <ncurses.h>

#include <audioguest.h>
#include <netdb.h>
#include <volume.h>
#include <visualiser.h>
#include <equaliser.h>
#include <guestutils.h>
#include <smplutils.h>
#include <ui.h>

float last3[2][3];
float last3_mod[10][2][3];

int fdw;
struct dest_infos server;
int token;
struct request req;
struct wav_params params;
struct settings usettings;
int is_clip;
int main(int argc, char* argv[]){

  initscr();
  raw();
  keypad(stdscr, TRUE);
  nodelay(stdscr, TRUE);
  noecho();
  curs_set(0);
  start_color();
  if(argc<3){
    printf("\nUsage : \n\n audioguest server_hostname file_name \n\n");
    endwin();
    clean_exit();
  }

  int wri;
  usettings.vol = 0;
  usettings.eq_on = 0;
  usettings.eq_ui = 1;
  usettings.vol_ui = 1;
  for(int a = 0; a < N_FILTERS; a++){
    usettings.eq_gains[a] = 0;
  }
  is_clip = 0;

  struct audio_packet packet;
  packet.header = 0;
  // last WIN_SIZE packets, for visualizing
  uint8_t window[WIN_SIZE][BUF_SIZE]; 

  req.token = 0;
  req.req_n = 1;
  strcpy(req.filename, argv[2]);
  //if (argv[3] == 0)
  req.mono = 1;
  int delay[] =  {1000,10000};             // between 2 writes to the speaker
                                           // attempts to prevent the padsp broken pipe error
                                           // mono : 10ms, stereo : 1ms
  socket_guest_init( &server, argv[1]);

  if (!send_packet(&req, sizeof(req), &server)){ 

    int info[4];
    recv_packet(info, sizeof(info), &server);

    if(info[0] == -1){            // dans le cas o� le filename n'est pas trouvé ar
      perror("Server says the file was not found"); //le serveur, celui ci renvoie
      clean_exit();                                    // -1 dans le champs reservéau 
    }                                             // channel.

    params.channels = info[2];
    params.sample_rate = info[0];
    params.sample_size = info[1];

    if(argc == 3){
      fdw = aud_writeinit(info[0],info[1],info[2]);
    }
    if(fdw < 0){
      perror("Could not get speaker's file descriptor");
    }       

    // getting a token assigned
    req.token = info[3];

    do{

      req_until_ack(&req, sizeof(req), &packet, sizeof(packet), &server);
      req.req_n += 1;

      get_input(&usettings);
      mvprintw(0,0,"%d", usettings.eq_on);
      if(usettings.eq_on)
        equalize(packet.audio, BUF_SIZE);

//      change_volume(packet.audio, BUF_SIZE);
  

      visualize_window(packet.audio, window, req.req_n);

      refresh();

      if(is_clip)
        clip();
      mvprintw(0,0,"%d", info[2]);

      float volume_db[2] = {0,0};
      mesure_volume(volume_db, &packet);
      display_volume(volume_db, &packet);

      refresh();
      
      draw_ui();
      draw_controls();
      refresh();

      if(argc == 3){
        wri = write(fdw, packet.audio , BUF_SIZE);
        if (wri < 0) {
          perror("Could not write to speaker");
          clean_exit();
        }
      }


      usleep(delay[req.mono]);
      //printf("%d \n", packet.header);

    }while(packet.header != -1);
    clean_exit();
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


int req_until_ack( struct request* req, short unsigned int rsize, struct audio_packet* packet, short unsigned int size, struct dest_infos* infos ){

  fd_set rfds;
  struct timeval tv;// = {5};
  int retval = 0;

  tv.tv_usec = 0;
  FD_ZERO(&rfds);

  // until the requested packet or the EOF is received
  while(packet->header != req->req_n && packet->header != -1){

    FD_ZERO(&rfds);
    FD_SET(infos->fd, &rfds);
    tv.tv_sec = 2;

    // send a request
    send_packet(req, rsize, infos);
    printf("requesting packet %d with token %d \n",req->req_n, req->token);


    // wait until timeout or packet received
    retval = select(infos->fd+1, &rfds, NULL, NULL, &tv);

    if (retval == -1){
      perror("select() failed");
      return 1;
    }
    else if ( (retval == 1) && FD_ISSET(infos->fd, &rfds))
      // packet received, read it
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
    clean_exit();
    exit(1); // makes the compiler happy
  }
  else {
    addr = (struct in_addr*) resolv->h_addr_list[0];
    //printf("The IP address of %s is %s\n",hostname,inet_ntoa(*addr));
    return inet_ntoa(*addr);
  }
}




void clip(){
  init_pair(1, COLOR_RED, COLOR_BLACK);
  attron(COLOR_PAIR(1));
  mvprintw(0,0,"CLIP");
  attroff(COLOR_PAIR(1));
  is_clip = 0;
}



void clean_exit(){

  struct audio_packet packet;
  
  if (req.token != 0){
    // connection to server is established
    // try to end it
    req.req_n = -1;
    req_until_ack(&req, sizeof(req), &packet, sizeof(packet), &server);
  }
  close(fdw);
  close(server.fd);
  printf("exiting... \n");
  sleep(1);                     // to be able to read perror before ncurses ends the window thus erasing the error message
  endwin();
  exit(1);

}

