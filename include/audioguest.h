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

int socket_guest_init( struct dest_infos* server, char* hostname);

int req_until_ack( struct request* req, short unsigned int rsize, struct audio_packet* packet, short unsigned int size, struct dest_infos* infos );

char* resolv_hostname(const char *hostname); 

void display_volume(float* volume_db, struct audio_packet* packet);

void mesure_volume(float* volume_db, struct audio_packet* packet);

void change_volume(struct audio_packet * packet, int user_volume);

void get_input(short int * user_volume);

void visualize_equalize(struct audio_packet * packet, float* gain);

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
