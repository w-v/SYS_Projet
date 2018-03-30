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

void display_volume(float* volume_db, struct wav_params* params, struct audio_packet* packet);

void mesure_volume(float* volume_db, struct wav_params* params, struct audio_packet* packet);

void change_volume(struct audio_packet * packet, struct wav_params * params, int user_volume);

void get_input(short int * user_volume);

void visualize_equalize(struct audio_packet * packet, struct wav_params * params, float* gain);

void visualize(double * channels_f[2], double channels_ft_out[2][256]);

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

};

void compute_apply_filter(struct filter_coeffs* fc, float sample, int ch);

void bpf(struct filter_coeffs* fc);

void hpf(struct filter_coeffs*  fc);

void lpf(struct filter_coeffs* fc);

void equalize(double* channels_f[2], struct wav_params * params, float gains[10]);

