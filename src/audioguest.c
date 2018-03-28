#include <audioguest.h>
int main(int argc, char* argv[]){

  initscr();
  raw();
  keypad(stdscr, TRUE);
  nodelay(stdscr, TRUE);
  noecho();
  curs_set(0);

  if(argc != 3){
    printf("\nUsage : \n\n audioguest server_hostname file_name \n\n");
    exit(1);
  }

  int wri;
  short int volume_user = 190;

  struct audio_packet packet;
  packet.header = 0;
  struct dest_infos server;
  struct request req;
  struct wav_params params;

  req.token = 0;
  req.req_n = 1;
  strcpy(req.filename, argv[2]);

  socket_guest_init( &server, argv[1]);

  if (!send_packet(&req, sizeof(req), &server)){ 

    int info[4];
    recv_packet(info, sizeof(info), &server);

    if(info[0] == -1){            // dans le cas o� le filename n'est pas trouvé ar
      perror("Server says the file was not found"); //le serveur, celui ci renvoie
      exit(1);                                     // -1 dans le champs reservéau
    }                                             // channel.

    params.channels = info[2];
    params.sample_rate = info[0];
    params.sample_size = info[1];

    int fdw = aud_writeinit(info[0],info[1],info[2]);
    if(fdw < 0){
      perror("Could not get speaker's file descriptor");
    }       

    // getting a token assigned
    req.token = info[3];

    do{

      req_until_ack(&req, sizeof(req), &packet, sizeof(packet), &server);
      req.req_n += 1;

      get_input(&volume_user);
      clear();
      change_volume(&packet, &params, volume_user);
  
      float volume_db[2] = {0,0};
      mesure_volume(volume_db, &params, &packet);
      display_volume(volume_db, &params, &packet);

      wri = write(fdw, packet.audio , BUF_SIZE);
      if (wri < 0) {
        perror("Could not write to speaker");
        exit(1);
      }


      //usleep(1000);
      //printf("%d \n", packet.header);

    }while(packet.header != -1);
    endwin();
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

  // until a packet is received and it is the requested one
  while(retval == 0 && !(FD_ISSET(infos->fd, &rfds)) && (packet->header != req->req_n)){

    FD_ZERO(&rfds);
    FD_SET(infos->fd, &rfds);
    tv.tv_sec = 2;

    // send a request
    send_packet(req, rsize, infos);
    // printf("requesting packet %d with token %d \n",req->req_n, req->token);


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
    exit(1);
  }
  else {
    addr = (struct in_addr*) resolv->h_addr_list[0];
    //printf("The IP address of %s is %s\n",hostname,inet_ntoa(*addr));
    return inet_ntoa(*addr);
  }
}


void display_volume(float* volume_db, struct wav_params* params, struct audio_packet* packet){
  int w, h;
  getmaxyx(stdscr, h, w);
  for(int ch = 0; ch < params->channels; ch++){
    int level = (volume_db[ch]+60)*(h/60.f);      // diplaying level from 0 to -60 dB
    mvprintw(3+ch,0,"ch: %d level: %d",ch,level);refresh();
    for(int i = 0; i < level; ++i){
      mvaddch(h - i, ch+40, 'M');
    }
  }
  refresh();
}

void mesure_volume(float* volume_db, struct wav_params* params, struct audio_packet* packet){



  // TODO : make another case for 8 bit signed, possibly 32 bit float
  const int max_amp = pow(2,params->sample_size-1); 
  int nsmpl = (BUF_SIZE / sizeof(int16_t)); 
  int16_t * smpl  = (int16_t *) packet->audio;
  int ch = 0;
  // find the maximum for each channel
  for (int i = 0; i < nsmpl; i++){
    
    if( smpl[i] > volume_db[ch] )
      volume_db[ch] = smpl[i];

    // switch to other channel
    ch = (ch + 1) % params->channels;
  }
  mvprintw(1,0,"vL: %f, vR: %f",volume_db[0],volume_db[1]);refresh();
  
  for(int ch = 0; ch < params->channels; ch++){             // for each channel 
    
    volume_db[ch] = 20.f*log10( volume_db[ch] / max_amp );      // convert sample value to dB level
  }
  mvprintw(2,0,"vL: %f, vR: %f",volume_db[0],volume_db[1]);refresh();

}

void change_volume(struct audio_packet * packet, struct wav_params * params, int user_volume){
  float db = (((user_volume)/10.f)-20.f);
  float att = pow(10, (db / 20.f));
  mvprintw(0,0,"u:%d db:%f att:%f",user_volume,db,att);

  // TODO : make another case for 8 bit signed, possibly 32 bit float
  int nsmpl = (BUF_SIZE / sizeof(int16_t)); 
  int16_t * smpl  = (int16_t *) packet->audio;

  for (int i = 0; i < nsmpl; i++){
    // change volume
    smpl[i] *= att;
  }


}

void get_input(short int * user_volume){
  int volume_delta = 5;
  int ch;
  switch (ch = getch()){
    case KEY_UP:
      *user_volume += volume_delta;
      break;
    case KEY_DOWN:
      *user_volume -= volume_delta;
      break;
    case 'q':
      endwin();
      exit(0);
      break;
    default:
      break;
  }
  if (*user_volume > 300)
    *user_volume = 300;
  else if (*user_volume < 0)
    *user_volume = 0;
}
