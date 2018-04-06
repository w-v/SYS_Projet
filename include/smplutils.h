#ifndef SMPL_UTILS_H
#define SMPL_UTILS_H

#include <stdint.h>

void char_to_float(uint8_t * audio, int audio_size, double ** audio_f);

void float_to_char(double ** audio_f, uint8_t * audio, int audio_size);

struct wav_params {

  unsigned short int channels;
  unsigned short int sample_size;
  int sample_rate;

};
#endif
