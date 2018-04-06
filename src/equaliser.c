#include <equaliser.h>
#include <guestutils.h>
#include <smplutils.h>
#include <math.h>
#include <malloc.h>
#include <string.h>


void equalize(uint8_t * audio, int audio_size){
  

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
    peak(&fc);
    for(int c = 0; c < nchans; c++){ 
      memcpy( &last3[c], &last3_orig[c], sizeof(last3[0]) );
      for(int s = 0; s < nsmpls; s++){
        compute_apply_filter(&fc, audio_f[c][s], c, f);
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
