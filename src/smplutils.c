#include <smplutils.h>
#include <guestutils.h>
#include <stdint.h>
#include <math.h>


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
      if (fabs(audio_f[c][s]) > 1){
        audio_f[c][s] = audio_f[c][s]/fabs(audio_f[c][s]); 
        is_clip=1;
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
