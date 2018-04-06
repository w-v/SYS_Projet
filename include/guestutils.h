#ifndef GUEST_UTILS_H
#define GUEST_UTILS_H

#include <stdint.h>
#include <equaliser.h>
#include <smplutils.h>

extern int fdw;
extern struct dest_infos server;
extern int token;
extern struct request req;
extern struct wav_params params;
extern struct settings usettings;
extern int is_clip;
extern void clean_exit();

struct settings {

  short int vol;
  uint8_t eq_on;
  uint8_t eq_ui;
  uint8_t vol_ui;
  int8_t eq_gains[N_FILTERS];
  
  uint8_t cursor;

};

#endif
