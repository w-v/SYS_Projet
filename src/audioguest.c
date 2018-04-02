#include <audioguest.h>
float last3[2][3];
float last3_mod[10][2][3];

int fdw;
struct dest_infos server;
int clip = 0;
struct wav_params params;
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
      
      equalize(packet.audio, BUF_SIZE);

      change_volume(&packet, volume_user);
  
      float volume_db[2] = {0,0};
      mesure_volume(volume_db, &packet);

      display_volume(volume_db, &packet);

      visualize(packet.audio, BUF_SIZE);

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


void display_volume(float* volume_db, struct audio_packet* packet){
  int h = getmaxy(stdscr);
  for(int ch = 0; ch < params.channels; ch++){
    int level = (volume_db[ch]+60)*(h/60.f);      // diplaying level from 0 to -60 dB
    //mvprintw(3+ch,0,"ch: %d level: %d",ch,level);refresh();
    for(int i = 0; i < level; ++i){
      mvaddch(h - i, ch+1, 'M');
    }
  }
  refresh();
}

void mesure_volume(float* volume_db, struct audio_packet* packet){


  if (params.channels == 2){

    // TODO : make another case for 8 bit signed, possibly 32 bit float
    const int max_amp = pow(2,params.sample_size-1); 
    int nsmpl = (BUF_SIZE / sizeof(int16_t)); 
    int16_t * smpl  = (int16_t *) packet->audio;
    int ch = 0;
    // find the maximum for each channel
    for (int i = 0; i < nsmpl; i++){

      if( smpl[i] > volume_db[ch] )
        volume_db[ch] = smpl[i];

      // switch to other channel
      ch = (ch + 1) % params.channels;
    }
    //mvprintw(1,0,"vL: %f, vR: %f",volume_db[0],volume_db[1]);refresh();

    for(int ch = 0; ch < params.channels; ch++){             // for each channel 

      volume_db[ch] = 20.f*log10( volume_db[ch] / max_amp );      // convert sample value to dB level
    }
    //mvprintw(2,0,"vL: %f, vR: %f",volume_db[0],volume_db[1]);refresh();
  }

}

void change_volume(struct audio_packet * packet, int user_volume){
  if (params.channels == 2){
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

void float_to_char(double ** audio_f, uint8_t * audio, int audio_size){
  const int max_amp = (pow(2, params.sample_size - 1) - 1); 
  // number of samples per channel
  int nsmpls = audio_size / (params.sample_size/8) / params.channels;

  int16_t * audio16;
  uint8_t * audio8;
  switch (params.sample_size){
    case 16:
      audio16 = (int16_t *) audio;
      break;
    case 8:
      audio8 = (uint8_t *) audio;
  }

  for(int c = 0; c < 2; c++){
    for (int s = 0; s < nsmpls; s ++){
      if (abs(audio_f[c][s]) > 1){
        clip++;
        audio_f[c][s] = 1; 
      }
      switch (params.sample_size){
        case 16:
          audio16[s*params.channels + c] = (audio_f[c][s]) * max_amp;
          break;
        case 8:
          audio8[s*params.channels + c] = (audio_f[c][s]) * max_amp;
      }
    }
  }
}

void char_to_float(uint8_t * audio, int audio_size, double ** audio_f){
  
  const int max_amp = (pow(2, params.sample_size - 1) - 1); 
  // number of samples per channel
  int nsmpls = audio_size / (params.sample_size/8) / params.channels;

  int16_t * audio16;
  uint8_t * audio8;
  switch (params.sample_size){
    case 16:
      audio16 = (int16_t *) audio;
      break;
    case 8:
      audio8 = (uint8_t *) audio;
  }

  for(int ch = 0; ch < params.channels; ch++){
    for (int s = 0; s < nsmpls; s++){
      switch (params.sample_size){
        case 16:
          audio_f[ch][s] = (audio16[s*params.channels+ch] / (double) max_amp);
          break;
        case 8:
          audio_f[ch][s] = (audio8[s*params.channels+ch] / (double) max_amp);
      }
    }
  }

}


void visualize(uint8_t * audio, int audio_size){
  int nchans = params.channels;
  // number of samples per channel
  int nsmpls = audio_size / (params.sample_size/8) / params.channels;
  double window;
  double ** fft_in = (double **) malloc(nchans*sizeof(double*));
  for (int ch = 0; ch < params.channels ; ch++){
    fft_in[ch] = fftw_alloc_real(nsmpls);
  }
  char_to_float(audio, audio_size, fft_in);
  for(int ch = 0; ch < nchans; ch++){
    for (int s = 0; s < nsmpls; s++){
      // Hann window (avoiding errors caused by the buffer being limited in size)
      window = 0.5f*( 1-cos( (2 * M_PI * s)/(nsmpls - 1) ) );
      fft_in[ch][s] *= window;
    }
  }
  fftw_complex * fft_out;
  double fft_bins[nsmpls/2];
  fftw_plan plan;
  for (int i = 0; i < nchans; i++){
    fft_out = fftw_alloc_complex(nsmpls);
    for (int j = 0; j < (nsmpls); j++){
      fft_out[j] = 0;
    }
    plan = fftw_plan_dft_r2c_1d(nsmpls, fft_in[i], fft_out, 0); 
    fftw_execute(plan);
    for (int j = 0; j < nsmpls; j++){
      // complex absolute value in place calculation
      // fft_in becomes the output
      fft_in[i][j] = cabs(fft_out[j]);
    }
    
  }
  
  // max between channels
  for (int j = 0; j < nsmpls / 2; j++){
    if(fft_in[0][j] < fft_in[1][j]){
      fft_bins[j] = 20 * log10(fft_in[1][j]);
    }
    else {
      fft_bins[j] = 20 * log10(fft_in[0][j]);
    }
  }

  for (int ch = 0; ch < params.channels ; ch++){
    fftw_free(fft_in[ch]);
  }
  fftw_free(fft_out);

  int w, h;
  getmaxyx(stdscr, h, w);
  w -= 3;
  h-= 1;
  
  clear();
  mvprintw(0,w-5,"clip:%d",clip);
  
  for(int i = 0; i < nsmpls / 2; i++){
    
    // average BUF_SIZE bins into w bars
    /*for(int a = 0; a < bin_per_bar; a++){
      bar_h += fft_bins[i*bin_per_bar+a] / bin_per_bar;
    } */
    
    int peak = h-(fft_bins[i]-20.f)*(-1.f)*(h/110.f);
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

void equalize(uint8_t * audio, int audio_size){
  

  /* TODO
   *
   * MEMCPY MUFUCKA !!!!!!!!!!!!!!!
   *
   */
  int nchans = params.channels;
  // number of samples per channel
  int nsmpls = audio_size / (params.sample_size/8) / params.channels;
  double ** audio_f = (double **) malloc(nchans*sizeof(double*));
  double ** audio_acc = (double **) malloc(nchans*sizeof(double*));
  for (int ch = 0; ch < nchans; ch++){
    audio_f[ch] = malloc(nsmpls*sizeof(double)); 
    audio_acc[ch] = malloc(nsmpls*sizeof(double)); 
  }
  char_to_float(audio, audio_size, audio_f);
  for(int c = 0; c < nchans; c++){
    for(int s = 0; s < nsmpls; s++){
      audio_acc[c][s] = 0;//audio[c][s];
    }
  }
  float ffreq[10] = {31.5, 63, 125,250,500,1000,2000,4000,8000,16000};
  //float gains[10] = {10,10,10,10,10,10,10,10,10,10};
  //float gains[10] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
  //float gains[10] = {5,5,5,5,5,5,5,5,5,5};
  //float gains[10] = {0,0,0,2,2,2,2,2,2,2};
  float gains[10] = {10,10,10,10,1,1,1,1,1,1};
  //                 o o o o o o x x x x 
  //float gains[10] = {-1,-1,-1,0,0,0,0,0,0,0};
  //float gains[10] = {0,0,0,0,0,0,1,0,0,0};
  float gains_offset[10]= {1,1,1,1,1,1,1,1,1};
  float qss[10] = {2,2,2,2,2,2,2,2,2,2};
  //float qss[10] = {0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5};
  //float gains_sum = 0; 
  for(int i = 0; i < 10; i++){
    gains[i] *= gains_offset[i];
    //gains_sum += abs(gains[i]);
  }
  
  struct filter_coeffs fc;
  float w0;
  
  
  float last3_orig[2][3];

  for(int c = 0; c < nchans; c++){
      memcpy( &last3_orig[c], &last3[c], sizeof(last3[0]) );
  }
  
  

  for(int f = 0; f < 7; f++){
    w0 = 2 * M_PI * ffreq[f] / params.sample_rate;
    fc.cos_w0 = cos(w0);
    fc.sin_w0 = sin(w0);
    fc.alpha = fc.sin_w0 / (2 * qss[f]);
    fc.A = pow(10, ( ( 20*log10(gains[f]) ) /40 ));//0.31;//sqrt(gains[f]);
    if (f == 18)
      hpf(&fc);
    else if (f == 19)
      lpf(&fc);
    else
      bpf(&fc);
    for(int c = 0; c < nchans; c++){ 
      memcpy( &last3[c], &last3_orig[c], sizeof(last3[0]) );
      for(int s = 0; s < nsmpls; s++){
        compute_apply_filter(&fc, audio_f[c][s], c, f);
        // parallel
        audio_acc[c][s] = ( audio_acc[c][s] + (last3_mod[f][c][0] * gains[f]) ) ;//* 0.5;
        // cascade
        //audio_f[c][s] = last3_mod[f][c][0];// * gains[f];
      }
    }
  }

  for(int c = 0; c < nchans; c++){
    for(int s = 0; s < nsmpls; s++){
      audio_f[c][s] = audio_acc[c][s]*0.1;
    }
  }
  
  float_to_char(audio_f, audio, audio_size);
  for (int ch = 0; ch < nchans; ch++){
    free(audio_f[ch]);
    free(audio_acc[ch]);
  }
  free(audio_f);
  free(audio_acc);
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

void notch(struct filter_coeffs* fc){
  fc->b0 = 1;
  fc->b1 = -2*fc->cos_w0;
  fc->b2 = 1;
  fc->a0 = 1 + fc->alpha;
  fc->a1 = -2*fc->cos_w0;
  fc->a2 = 1 - fc->alpha;
}

void peak(struct filter_coeffs* fc){
  fc->b0 = 1 + fc->alpha*fc->A;
  fc->b1 = -2*fc->cos_w0;
  fc->b2 = 1 - fc->alpha*fc->A;
  fc->a0 = 1 + fc->alpha/fc->A;
  fc->a1 = -2*fc->cos_w0;
  fc->a2 = 1 - fc->alpha/fc->A;
}

void compute_apply_filter(struct filter_coeffs* fc, float sample, int ch, int f){
  last3[ch][2] = last3[ch][1];
  last3[ch][1] = last3[ch][0];
  last3[ch][0] = sample;
  last3_mod[f][ch][2] = last3_mod[f][ch][1];
  last3_mod[f][ch][1] = last3_mod[f][ch][0];
  last3_mod[f][ch][0] = (fc->b0 / fc->a0 * last3[ch][0]) +
  (fc->b1 / fc->a0 * last3[ch][1]) +
  (fc->b2 / fc->a0 * last3[ch][2]) -
  (fc->a1 / fc->a0 * last3_mod[f][ch][1]) -
  (fc->a2 / fc->a0 * last3_mod[f][ch][2]);
}
