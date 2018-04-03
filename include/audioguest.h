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
#include <ncurses.h>

#include <netdb.h>

#include <socketlib.h>

#define WIN_SIZE        2
#define LOW_FREQ        25

#define N_FILTERS       10      
#define EQ_MAX_GAIN     40

#define CURS_VOL         0
#define CURS_EQ          1

#define EQ_UI_W         40
#define EQ_UI_H         20

#define MAX_VOL         300
#define VOL_STEP        5
int socket_guest_init( struct dest_infos* server, char* hostname);

int req_until_ack( struct request* req, short unsigned int rsize, struct audio_packet* packet, short unsigned int size, struct dest_infos* infos );

char* resolv_hostname(const char *hostname); 

void display_volume(float* volume_db, struct audio_packet* packet);

void mesure_volume(float* volume_db, struct audio_packet* packet);

void change_volume(struct audio_packet * packet, int user_volume);

struct settings {

  short int vol;
  uint8_t eq_on;
  uint8_t eq_ui;
  int8_t eq_gains[N_FILTERS];
  
  uint8_t cursor;

};

void log_scale(double * unscaled, int size_unscaled, double * scaled, int size_scaled);

void get_input();

void draw_ui();

void update_settings(int d, int c);

void visualize_window(uint8_t * crt_packet, uint8_t window[WIN_SIZE][BUF_SIZE], int packet_id);

void visualize(uint8_t * audio, int audio_size);

void clean_exit();

struct filter_coeffs {

  float b0;
  float b1;
  float b2;
  float a0;
  float a1;
  float a2;

  float alpha;
  float cos_w0;
  float sin_w0;
  float A;

};

void compute_apply_filter(struct filter_coeffs* fc, float sample, int ch, int f);

void bpf(struct filter_coeffs* fc);

void hpf(struct filter_coeffs*  fc);

void lpf(struct filter_coeffs* fc);

void peak(struct filter_coeffs* fc);

void notch(struct filter_coeffs* fc);

void equalize(uint8_t * audio, int audio_size);

void char_to_float(uint8_t * audio, int audio_size, double ** audio_f);

void float_to_char(double ** audio_f, uint8_t * audio, int audio_size);

