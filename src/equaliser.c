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

  // convert to float
  char_to_float(audio, audio_size, audio_f);

  // fill the initial sum with the original signal
  for(int c = 0; c < nchans; c++){
    for(int s = 0; s < nsmpls; s++){
      audio_acc[c][s] = audio_f[c][s];
    }
  }

  float gains[10]= {1,1,1,1,1,1,1,1,1,1};
  for(int i = 0; i < N_FILTERS; i++)
    gains[i] = pow(10, usettings.eq_gains[i] /20.f );   // convert from dB
  
  // filter frequencies
  float ffreq[10] = {31.5, 63, 125,250,500,1000,2000,4000,8000,16000};
  float qss[10] = {2,2,2,2,2,2,2,2,2,2};

  // parallel or cascade application of filters
  int parallel = 1;

  struct filter_coeffs fc;
  float w0;

  // last 3 original samples
  float last3_orig[2][3];

  // remember last 3 of last packet
  for(int c = 0; c < nchans; c++){
    memcpy( &last3_orig[c], &last3[c], sizeof(last3[0]) );
  }

  // apply each filter
  for(int f = 0; f < N_FILTERS; f++){
    
    // coeffs
    w0 = 2 * M_PI * ffreq[f] / params.sample_rate;
    fc.cos_w0 = cos(w0);
    fc.sin_w0 = sin(w0);
    fc.alpha = fc.sin_w0 / (2 * qss[f]);
    fc.A = gains[f];

    // compute coeffs
    peak(&fc);

    // apply filter
    for(int c = 0; c < nchans; c++){       

      // restore last free of last packet
      memcpy( &last3[c], &last3_orig[c], sizeof(last3[0]) );
      for(int s = 0; s < nsmpls; s++){

        // compute new value
        compute_apply_filter(&fc, audio_f[c][s], c, f);
        if(parallel)
          // add it to the sum
          audio_acc[c][s] = ( audio_acc[c][s] + (last3_mod[f][c][0] ));
        else
          audio_f[c][s] = last3_mod[f][c][0];
      }
    }
  }

  for(int c = 0; c < nchans; c++){
    for(int s = 0; s < nsmpls; s++){
      if(parallel)
        // average all summed signals
        audio_f[c][s] = audio_acc[c][s] / N_FILTERS;    
      else
        audio_f[c][s] = audio_f[c][s];
    }
  }

  // convert back to original format (int16_t or uint8_t)
  float_to_char(audio_f, audio, audio_size);

  for (int ch = 0; ch < nchans; ch++){
    free(audio_f[ch]);
    free(audio_acc[ch]);
  }
  free(audio_f);
  free(audio_acc);
}

// band pass filter
void bpf(struct filter_coeffs* fc){
  fc->b0 = fc->sin_w0 / 2;
  fc->b1 = 0;
  fc->b2 = (-1)*fc->sin_w0 / 2;
  fc->a0 = 1 + fc->alpha;
  fc->a1 = (-2) * fc->cos_w0;
  fc->a2 = 1 - fc->alpha; 
}

// high pass filter
void hpf(struct filter_coeffs*  fc){
  fc->a0 = 1 + fc->alpha; 
  fc->a1 = (-2) * fc->cos_w0; 
  fc->a2 = 1 - fc->alpha;
  fc->b0 = (1 + fc->cos_w0) / 2;
  fc->b1 = -(1 + fc->cos_w0);
  fc->b2 = (1 + fc->cos_w0) / 2;
}

// low pass filter
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
