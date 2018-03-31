#include <audioguest.h>
float last_3_smpls[2][3];
float last_3_modified_smpls[10][2][3];

int fdw;
struct dest_infos server;
int clip = 0;
int main(int argc, char* argv[]){

  initscr();
  raw();
  keypad(stdscr, TRUE);
  nodelay(stdscr, TRUE);
  noecho();
  curs_set(0);

  if(argc<3){
    printf("\nUsage : \n\n audioguest server_hostname file_name \n\n");
    endwin();
    clean_exit();
  }

  int wri;
  short int volume_user = 190;
  //float qs[10] = {12000000,12000000,12000000,12000000,12000000,12000000,12000000,12000000,12000000,12000000};
//  float qs[10] = {1,12,12,12,12,12,12,12,12,1};
    float qs[10] = {2,2,2,2,2,2,2,2,2,2};

  struct audio_packet packet;
  packet.header = 0;
  struct request req;
  struct wav_params params;

  req.token = 0;
  req.req_n = 1;
  strcpy(req.filename, argv[2]);
  //if (argv[3] == 0)
  req.mono = 0;
  int delay[] =  {1000,10500};             // mono : 10ms, stereo : 1ms
  //else
  //  req.mono = 1;

  socket_guest_init( &server, argv[1]);

  if (!send_packet(&req, sizeof(req), &server)){ 

    int info[4];
    recv_packet(info, sizeof(info), &server);

    if(info[0] == -1){            // dans le cas o� le filename n'est pas trouvé ar
      perror("Server says the file was not found"); //le serveur, celui ci renvoie
      clean_exit();                                    // -1 dans le champs reservéau 
    }                                             // channel.

    //mvprintw(0,0,"%d", info[2]);
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

      get_input(&volume_user);
      change_volume(&packet, &params, volume_user);
  
      float volume_db[2] = {0,0};
      mesure_volume(volume_db, &params, &packet);

      visualize_equalize(&packet, &params, qs);

      if(argc == 3){
        wri = write(fdw, packet.audio , BUF_SIZE);
        if (wri < 0) {
          perror("Could not write to speaker");
          clean_exit();
        }
      }
      display_volume(volume_db, &params, &packet);


      usleep(delay[req.mono]);
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
    clean_exit();
    exit(1); // makes the compiler happy
  }
  else {
    addr = (struct in_addr*) resolv->h_addr_list[0];
    //printf("The IP address of %s is %s\n",hostname,inet_ntoa(*addr));
    return inet_ntoa(*addr);
  }
}


void display_volume(float* volume_db, struct wav_params* params, struct audio_packet* packet){
  int h = getmaxy(stdscr);
  for(int ch = 0; ch < params->channels; ch++){
    int level = (volume_db[ch]+60)*(h/60.f);      // diplaying level from 0 to -60 dB
    //mvprintw(3+ch,0,"ch: %d level: %d",ch,level);refresh();
    for(int i = 0; i < level; ++i){
      mvaddch(h - i, ch+1, 'M');
    }
  }
  refresh();
}

void mesure_volume(float* volume_db, struct wav_params* params, struct audio_packet* packet){


  if (params->channels == 2){

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
    //mvprintw(1,0,"vL: %f, vR: %f",volume_db[0],volume_db[1]);refresh();

    for(int ch = 0; ch < params->channels; ch++){             // for each channel 

      volume_db[ch] = 20.f*log10( volume_db[ch] / max_amp );      // convert sample value to dB level
    }
    //mvprintw(2,0,"vL: %f, vR: %f",volume_db[0],volume_db[1]);refresh();
  }

}

void change_volume(struct audio_packet * packet, struct wav_params * params, int user_volume){
  if (params->channels == 2){
    float db = (((user_volume)/10.f)-20.f);
    float att = pow(10, (db / 20.f));
    //mvprintw(0,0,"u:%d db:%f att:%f",user_volume,db,att);

    // TODO : make another case for 8 bit signed, possibly 32 bit float
    int nsmpl = (BUF_SIZE / sizeof(int16_t)); 
    int16_t * smpl  = (int16_t *) packet->audio;

    for (int i = 0; i < nsmpl; i++){
      // change volume
      smpl[i] *= att;
    }
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

void visualize_equalize(struct audio_packet * packet, struct wav_params * params, float qs[10]){
  if (params->channels == 2){
    int chan = 0;
    double * channels_f[2];
    double channels_ft_out[2][256];
    const double max_amp = (pow(2, params->sample_size - 1) - 1); 
    double window;                                      
    int nsmpl = (BUF_SIZE / sizeof(int16_t)); 
    int16_t * smpl  = (int16_t *) packet->audio;
    for (int i = 0; i < 2; i++){
      channels_f[i] = fftw_alloc_real(nsmpl / 2);
    }
    for (int i = 0; i < nsmpl; i++){
      window = 0.5f*( 1-cos( (2 * M_PI * i)/(nsmpl - 1) ) );                         // Hann window 
      channels_f[chan][i/2] = (smpl[i] / max_amp);// * window ;
      chan = (chan + 1) % 2;
    }
    equalize(channels_f,params, qs);
    for(int c = 0; c < 2; c++){
      for (int s = 0; s < 512; s += 2){
        int16_t tmp = (channels_f[c][s/2]) * max_amp;
          if (abs(tmp) < max_amp)
            smpl[s + c] = tmp;
          else if (tmp > 0){
            clip++;
            smpl[s + c] = max_amp; 
          }
          else {
            clip++;
            smpl[s + c] = (-1) * max_amp; 
          }
      }
    }
    visualize(channels_f,channels_ft_out);
  }
  /*else {
    float channels_FT [512];
    float channels_f [1024];
    for (int i = 0; i < BUF_SIZE; i++){
      channels_f [packet->audio
    }
    visualizer();
    equalizer();
  }*/
}

void visualize(double * channels_f[2], double channels_ft_out[2][256]){
  int nsmpl = (BUF_SIZE / sizeof(int16_t)); 
  fftw_complex * channels_FT;
  double channels_ft_out_db[128];
  fftw_plan plan;
  for (int i = 0; i < 2; i++){
    channels_FT = fftw_alloc_complex(nsmpl/2);
    for (int j = 0; j < (nsmpl / 2); j++){
      channels_FT[j] = 0;
    }
    plan = fftw_plan_dft_r2c_1d(nsmpl/2, channels_f[i], channels_FT, 0); 
    fftw_execute(plan);
    for (int j = 0; j < nsmpl / 2; j++){
      channels_ft_out[i][j] = cabs(channels_FT[j]);
    }
    
  }
  
  for (int j = 0; j < nsmpl / 4; j++){
    if(channels_ft_out[0][j] < channels_ft_out[1][j]){
      channels_ft_out_db[j] = 20 * log10(channels_ft_out[0][j]);
    }
    else {
      channels_ft_out_db[j] = 20 * log10(channels_ft_out[0][j]);
    }
  }

  int w, h;
  getmaxyx(stdscr, h, w);
  w -= 3;
  h-= 1;
  
  clear();
  mvprintw(0,w-5,"clip:%d",clip);
  
  for(int i = 0; i < nsmpl / 4; i++){
    
    // average BUF_SIZE bins into w bars
    /*for(int a = 0; a < bin_per_bar; a++){
      bar_h += channels_ft_out_db[i*bin_per_bar+a] / bin_per_bar;
    } */
    
    int peak = h-(channels_ft_out_db[i]-20.f)*(-1.f)*(h/110.f);
    for(int r = 0; r < peak;r++ ){
      
      mvaddch(h-r,i+4, 'W');

    }


  }
  refresh();
  
}

void clean_exit(){

  close(fdw);
  close(server.fd);
  printf("exiting... \n");
  sleep(1);                     // to be able to read perror before ncurses ends the window thus erasing the error message
  endwin();
  exit(1);

}

void equalize(double* channels_f[2], struct wav_params * params, float qs[10]){
  

  /* TODO
   *
   * MEMCPY MUFUCKA !!!!!!!!!!!!!!!
   *
   */

  double channels_acc[2][256];
  for(int c = 0; c < 2; c++){
    for(int s = 0; s < 256; s++){
      channels_acc[c][s] = channels_f[c][s];
    }
  }
  float ffreq[10] = {31.5, 63, 125,250,500,1000,2000,4000,8000,16000};
  float gains[10] = {1,1,1,1,1,1,1,1,1,1};
  
  struct filter_coeffs fc;
  float w0;
  
  
  float last_3_smpls_orig[2][3];

  for(int c = 0; c < 2; c++){
    for(int s = 0; s < 3; s++){
      last_3_smpls_orig[c][s] = last_3_smpls[c][s];
    }
  }
  
  

  for(int f = 0; f < 10; f++){

    w0 = 2 * M_PI * ffreq[f] / params->sample_rate;
    fc.cos_w0 = cos(w0);
    fc.sin_w0 = sin(w0);
    fc.alpha = fc.sin_w0 / (2 * qs[f]);
    if (f == 0)
      hpf(&fc);
    else if (f == 9)
      lpf(&fc);
    else
      bpf(&fc);
    for(int c = 0; c < params->channels; c++){ 
      for(int s = 0; s < 256; s++){
        compute_apply_filter(&fc, channels_f[c][s], c, f);
        // parallel
        channels_acc[c][s] = channels_acc[c][s] + (last_3_modified_smpls[f][c][0]
        * 1.8);
        // cascade
        //channels_f[c][s] = last_3_modified_smpls[c][0];
      }
    }
    for(int c = 0; c < 2; c++){
      for(int s = 0; s < 3; s++){
        last_3_smpls[c][s] = last_3_smpls_orig[c][s];
      }
    }
  }

  for(int c = 0; c < 2; c++){
    for(int s = 0; s < 256; s++){
      channels_f[c][s] = channels_acc[c][s] * 0.01;
    }
  }
  
}

void bpf(struct filter_coeffs* fc){
  fc->b0 = fc->sin_w0 / 2;
  fc->b1 = 0;
  fc->b2 = (-1)*fc->sin_w0 / 2;
  fc->a0 = 1 + fc->alpha;
  fc->a1 = (-2) * fc->cos_w0;
  fc->a2 = 1 - fc->alpha; 
}
void hpf(struct filter_coeffs*  fc){
  fc->a0 = 1 + fc->alpha; 
  fc->a1 = (-2) * fc->cos_w0; 
  fc->a2 = 1 - fc->alpha;
  fc->b0 = (1 + fc->cos_w0) / 2;
  fc->b1 = -(1 + fc->cos_w0);
  fc->b2 = (1 + fc->cos_w0) / 2;
}
void lpf(struct filter_coeffs* fc){
  fc->b0 =  (1 - fc->cos_w0)/2;
  fc->b1 =   1 - fc->cos_w0;
  fc->b2 =  (1 - fc->cos_w0)/2;
  fc->a0 =   1 + fc->alpha;
  fc->a1 =  (-2)*fc->cos_w0;
  fc->a2 =   1 - fc->alpha;
}

void compute_apply_filter(struct filter_coeffs* fc, float sample, int ch, int f){
  last_3_smpls[ch][2] = last_3_smpls[ch][1];
  last_3_smpls[ch][1] = last_3_smpls[ch][0];
  last_3_smpls[ch][0] = sample;
  last_3_modified_smpls[f][ch][2] = last_3_modified_smpls[f][ch][1];
  last_3_modified_smpls[f][ch][1] = last_3_modified_smpls[f][ch][0];
  last_3_modified_smpls[f][ch][0] = (fc->b0 / fc->a0 * last_3_smpls[ch][0]) +
  (fc->b1 / fc->a0 * last_3_smpls[ch][1]) +
  (fc->b2 / fc->a0 * last_3_smpls[ch][2]) -
  (fc->a1 / fc->a0 * last_3_modified_smpls[f][ch][1]) -
  (fc->a2 / fc->a0 * last_3_modified_smpls[f][ch][2]);
}
