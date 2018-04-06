#ifndef EQUALISER_H
#define EQUALISER_H

#include <stdint.h>

#define N_FILTERS       10      
#define EQ_MAX_GAIN     20

extern float last3[2][3];
extern float last3_mod[10][2][3];

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

#endif
