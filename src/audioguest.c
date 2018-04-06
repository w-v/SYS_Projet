#include <audioguest.h>
#include <fftw3.h>

float last3[2][3];
float last3_mod[10][2][3];

int fdw;
struct dest_infos server;
int clip = 0;
struct wav_params params;
struct settings usettings;
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
  usettings.vol = 0;
  usettings.eq_on = 0;
  usettings.eq_ui = 1;
  usettings.vol_ui = 1;
  for(int a = 0; a < N_FILTERS; a++){
    usettings.eq_gains[a] = 0;
  }

  struct audio_packet packet;
  packet.header = 0;
  struct request req;
  // last WIN_SIZE packets, for visualizing
  uint8_t window[WIN_SIZE][BUF_SIZE]; 

  req.token = 0;
  req.req_n = 1;
  strcpy(req.filename, argv[2]);
  //if (argv[3] == 0)
  req.mono = 0;
  int delay[] =  {1000,10500};             // between 2 writes to the speaker
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

      get_input(&usettings);
      mvprintw(0,0,"%d", usettings.eq_on);
      if(usettings.eq_on)
        equalize(packet.audio, BUF_SIZE);

      change_volume(packet.audio, BUF_SIZE);
  

      visualize_window(packet.audio, window, req.req_n);
      refresh();
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


void display_volume(float* volume_db, struct audio_packet* packet){
  int h = getmaxy(stdscr)-3;
  mvprintw(h+1,1,"LR");
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

void change_volume(uint8_t * audio, int audio_size){
  float vol_gain = pow(10, (usettings.vol / 20.f));
  int16_t * audio16;
  if (params.sample_size == 16)
    audio16 = (int16_t *) audio;

  for (int s = 0; s < audio_size; s ++){
    switch (params.sample_size){
      case 16:
        audio16[s] *= vol_gain;
        break;
      case 8:
        audio[s] *= vol_gain;
    }
  }
}


void get_input(){
  int ch;
  switch (ch = getch()){
    case KEY_LEFT:
      usettings.cursor--;
      break;
    case KEY_RIGHT:
      usettings.cursor++;
      break;
    case KEY_UP:
      update_settings(+1, usettings.cursor);
      break;
    case KEY_DOWN:
      update_settings(-1, usettings.cursor);
      break;
    case '+':
      update_settings(+1, CURS_VOL);
      break;
    case '-':
      update_settings(-1, CURS_VOL);
      break;
    case 'b':
      usettings.eq_on = !usettings.eq_on;
      break;
    case 'e':
      usettings.eq_ui = !usettings.eq_ui;
      break;
    case 'v':
      usettings.vol_ui = !usettings.vol_ui;
      break;
    case 'q':
      clean_exit();
      break;
    default:
      break;
  }
}

void update_settings(int d, int c){
  switch(c){
    case CURS_VOL:
      usettings.vol+=VOL_STEP*d;
      if (usettings.vol > MAX_VOL)
        usettings.vol = MAX_VOL;
      else if (usettings.vol < -MAX_VOL)
        usettings.vol = -MAX_VOL;
      break;
    default:
      usettings.eq_gains[c-CURS_EQ] += (EQ_MAX_GAIN*2/EQ_UI_H)*d;
      if (usettings.eq_gains[c-CURS_EQ] > EQ_MAX_GAIN)
        usettings.eq_gains[c-CURS_EQ] = EQ_MAX_GAIN;
      else if (usettings.eq_gains[c-CURS_EQ] < -EQ_MAX_GAIN)
        usettings.eq_gains[c-CURS_EQ] = -EQ_MAX_GAIN;
  }
}

void draw_controls(){
  int w, h, y, x;
  getmaxyx(stdscr, h, w);
  x = 4;
  mvprintw(h-2,x,"EQ ON/OFF: B(ypass)\tSHOW EQ UI: E(Q)\tSHOW VOL UI: V(olume)\tSET VOL: +/-\n");
  mvprintw(h-1,x,"NAVIGATE UI: LEFT/RIGHT\tSET GAINS: UP/DOWN\tEXIT: Q(uit)\n");
  
}

void draw_ui(){

  int w, h, y, x;
  getmaxyx(stdscr, h, w);
  w-=WIN_PADDING;
  h-=WIN_PADDING;
  x = w;
  y = WIN_PADDING;
  const char *scaley[5] = { "+20dB ", "+10dB ", "  0dB ", "-10dB ", "-20dB " };
  const char *scalex[N_FILTERS] = {" 30 ", " 65 ", "125 ", "250 ","500 ", " 1k ", " 2k ", " 4k ", " 8k ", "16k "};
  int bar_h;  
  int bar_w = EQ_UI_W/N_FILTERS;
  int f,a,c;
  char ch;
  float d = EQ_UI_H/(EQ_MAX_GAIN*2.f);
  char eq_blanks[EQ_UI_W+EQ_SCALE_W+UI_PADDING+1];
  char vol_blanks[VOL_UI_W+UI_PADDING+1];
  // TODO : clear whats under
  if(usettings.eq_ui){
    x-= EQ_UI_W+EQ_SCALE_W;
    memset(&eq_blanks, ' ', sizeof(eq_blanks));
    eq_blanks[EQ_UI_W+EQ_SCALE_W+UI_PADDING] = '\0';
    for(f = 0; f < EQ_UI_H+UI_PADDING+WIN_PADDING; f++)
      mvprintw(y+f,x-1,"%s",eq_blanks);
    for(int i = 0; i < 5; i++)
      mvprintw(y+EQ_UI_H*i/4.f,x,"%s",scaley[i]);
    x+=EQ_SCALE_W;
    for(int i = 0; i < N_FILTERS; i++){
      mvprintw(y+EQ_UI_H+1,x+i*4,"%s",scalex[i]);
    }
    for(f = 0; f < N_FILTERS; f++){
      bar_h = ( EQ_MAX_GAIN+usettings.eq_gains[f] )*d;

      for(a = 0; a < bar_h+1; a++){

        if(usettings.cursor == CURS_EQ+f)
          ch = 'O';
        else
          ch = 'H';

        for(c = 0; c < bar_w; c++){
          mvaddch(y+EQ_UI_H-a,x+f*bar_w+c,ch);
        }

      }

    }
    x-= EQ_SCALE_W;
  } 
  if(usettings.vol_ui){
    x-= VOL_UI_W+UI_PADDING;
    memset(&vol_blanks, ' ', sizeof(vol_blanks));
    vol_blanks[VOL_UI_W+UI_PADDING] = '\0';
    for(f = 0; f < VOL_UI_H+3; f++)
      mvprintw(y+f,x-1,"%s",vol_blanks);
    mvprintw(y+VOL_UI_H+1,x,"VOL");
    d = ( VOL_UI_H/(MAX_VOL*2.f) );
    bar_h = ( MAX_VOL + usettings.vol ) * d;
    for(a = 0; a < bar_h; a++){
      if(a < bar_h+1){
        if(usettings.cursor == CURS_VOL)
          ch = 'O';
        else
          ch = 'H';
      }
      else {
        ch = ' ';
      }
      for(c = 0; c < VOL_UI_W; c++){
        mvaddch(y+VOL_UI_H-a,x+c,ch);
      }
    }

  }

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

  for(int c = 0; c < params.channels; c++){
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

void visualize_window(uint8_t * crt_packet, uint8_t window[WIN_SIZE][BUF_SIZE], int packet_id){

  // copy current packet at the end of the window
  memcpy(&window[WIN_SIZE - 1], crt_packet, sizeof(window[0]));
  if(packet_id > 1)
    visualize((uint8_t *) window+BUF_SIZE*1, BUF_SIZE*(WIN_SIZE-1));

  // shift window to the right (packets towards the start of the array)
  for(int p = WIN_SIZE-1; p > 0; p--){
    memcpy(&window[p-1], &window[p], sizeof(window[0]));        // nasty stuff
  }

}

void visualize(uint8_t * audio, int audio_size){
  int nchans = params.channels;
  // number of samples per channel
  int nsmpls = audio_size / (params.sample_size/8) / params.channels;
  double window;
  int w, h;
  getmaxyx(stdscr, h, w);
  w -= 3;
  h-= 3;
  double * fft_bins = (double *) malloc(sizeof(double)*w);
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
  fftw_plan plan;
  for (int i = 0; i < nchans; i++){
    fft_out = fftw_alloc_complex(nsmpls);
    for (int j = 0; j < (nsmpls); j++){
      fft_out[j] = 0;
    }
    plan = fftw_plan_dft_r2c_1d(nsmpls, fft_in[i], fft_out, 0); 
    fftw_execute(plan);
    for (int j = 0; j < nsmpls; j++){
      // complex absolute value calculation
      // fft_in becomes the output
      fft_in[i][j] = cabs(fft_out[j]);
    }
    
  }
  fftw_free(fft_out);
  
  // max between channels
  for (int j = 0; j < nsmpls /2; j++){
    if(fft_in[0][j] < fft_in[1][j]){
      fft_in[0][j] = fft_in[1][j];
    }
  }
  
  log_scale(fft_in[0], nsmpls/2, fft_bins, w);
  clear();
  mvprintw(0,w-5,"clip:%d",clip);
  
  for(int i = 0; i < w-2; i++){
    
    //fft_bins[i] = 20 * log10(fft_in[0][i]);
    // average BUF_SIZE bins into w bars
    /*for(int a = 0; a < bin_per_bar; a++){
      bar_h += fft_bins[i*bin_per_bar+a] / bin_per_bar;
    } */
    
    int peak = h-(fft_bins[i]-30.f)*(-1.f)*(h/80.f);
    for(int r = 0; r < peak;r++ ){
      
      mvaddch(h-r,i+4, 'W');

    }

  }
  for (int ch = 0; ch < params.channels ; ch++){
    fftw_free(fft_in[ch]);
  }
  free(fft_in);
  free(fft_bins);
  refresh();
  
}

void log_scale(double * unscaled, int size_unscaled, double * scaled, int size_scaled){
  // frequency covered by one unscaled bin
  double bin_range = params.sample_rate / 2 / (double) size_unscaled;
  // constant for mapping frequency range (LOW_FREQHz to Nyquist freq.) into window
  double c = (params.sample_rate/2/LOW_FREQ);
  int s = 0;
  double tg;
  double tgg;
  scaled[0] = 0;
  for(int u = 0; u < size_unscaled; u++){
    tg = LOW_FREQ*pow(c, (s/(double)size_scaled));
    tgg = bin_range*u;
    if( tgg > tg ){
      if(scaled[s] != 0)
        scaled[s] = 20 * log10(scaled[s]);
      s++;
      if(s >= size_scaled)
        return;
      scaled[s] = 0;
    }
    scaled[s] += unscaled[u];
    
  }
  
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
      audio_acc[c][s] = audio_f[c][s];
    }
  }
  float ffreq[10] = {31.5, 63, 125,250,500,1000,2000,4000,8000,16000};
  float gains[10]= {1,1,10,10,10,1,1,1,1,1};
  float qss[10] = {2,2,2,2,2,2,2,2,2,2};
  float gains_sum = 0; 
  int parallel = 1;
  for(int i = 0; i < 10; i++){
    //gains[i] *= gains_offset[i];
    gains[i] = pow(10, usettings.eq_gains[i] /20.f );
    gains_sum += gains[i];
  }

  struct filter_coeffs fc;
  float w0;


  float last3_orig[2][3];

  for(int c = 0; c < nchans; c++){
    memcpy( &last3_orig[c], &last3[c], sizeof(last3[0]) );
  }



  for(int f = 0; f < 10; f++){
    w0 = 2 * M_PI * ffreq[f] / params.sample_rate;
    fc.cos_w0 = cos(w0);
    fc.sin_w0 = sin(w0);
    fc.alpha = fc.sin_w0 / (2 * qss[f]);
    fc.A = gains[f];//0.31;//sqrt(gains[f]);
    if (f == 20)
      lpf(&fc);
    else if (f == 29)
      hpf(&fc);
    else
      peak(&fc);
    for(int c = 0; c < nchans; c++){ 
      memcpy( &last3[c], &last3_orig[c], sizeof(last3[0]) );
      for(int s = 0; s < nsmpls; s++){
        compute_apply_filter(&fc, audio_f[c][s], c, f);
        // parallel
        //if(f == 0 || f == 9)
        //  audio_acc[c][s] = ( audio_acc[c][s] + (last3_mod[f][c][0] * gains[f]) ) ;//* 0.5;
        //else
          //audio_acc[c][s] = ( audio_acc[c][s] + (last3_mod[f][c][0] ));//* gains[f]) ) ;//* 0.5;
        
        // cascade
        if(parallel)
          audio_acc[c][s] = ( audio_acc[c][s] + (last3_mod[f][c][0] ));
        else
          audio_f[c][s] = last3_mod[f][c][0];// * gains[f];
      }
    }
  }
  for(int c = 0; c < nchans; c++){
    for(int s = 0; s < nsmpls; s++){
      if(parallel)
        audio_f[c][s] = audio_acc[c][s] / N_FILTERS;// / (gains_sum*0.5);
      else
        audio_f[c][s] = audio_f[c][s];
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
