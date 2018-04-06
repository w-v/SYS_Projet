#include <volume.h>
#include <guestutils.h>
#include <math.h>
#include <stdint.h>
#include <ncurses.h>


void display_volume(float* volume_db, struct audio_packet* packet){
  int h = getmaxy(stdscr)-3;
  mvprintw(h+1,1,"LR");
  for(int ch = 0; ch < params.channels; ch++){
    int level = (volume_db[ch]+60)*(h/60.f);      // diplaying levels from 20 to -40 dB
    for(int i = 0; i < level; ++i){
      mvaddch(h - i, ch+1, 'M');
    }
  }
  refresh();
}

void mesure_volume(float* volume_db, struct audio_packet* packet){

  const float max_amp = (pow(2, params.sample_size - 1) - 1); 
  int nsmpl = (BUF_SIZE / (params.sample_size/8) ); 
  int16_t * audio16;
  switch (params.sample_size){
    case 16:
      audio16 = (int16_t *) packet->audio;
      break;
  }

  // find the maximum for each channel
  int ch = 0;
  for (int s = 0; s < nsmpl; s ++){
    switch (params.sample_size){
      case 16:
        if( audio16[s] > volume_db[ch] )
          volume_db[ch] = audio16[s];
        break;
      case 8:
        if( packet->audio[s] > volume_db[ch] )
          volume_db[ch] = packet->audio[s];
    }
    // switch to other channel
    ch = (ch + 1) % params.channels;
  }

  for(int ch = 0; ch < params.channels; ch++){             // for each channel 
    volume_db[ch] = 20.f*log10( volume_db[ch] / max_amp );      // convert sample value to dB level
  }

}

void change_volume(uint8_t * audio, int audio_size){
  float vol_gain = pow(10, (usettings.vol / 20.f));
  const int max_amp = (pow(2, params.sample_size - 1) - 1); 
  int16_t * audio16;
  if (params.sample_size == 16)
    audio16 = (int16_t *) audio;

  for (int s = 0; s < audio_size; s ++){
    switch (params.sample_size){
      case 16:
        audio16[s] *= vol_gain;
        if(abs(audio16[s]) > max_amp){
          audio16[s] = max_amp;
          is_clip=1;
        }
        break;
      case 8:
        audio[s] *= vol_gain;
        if(audio[s] > max_amp){
          audio[s] = max_amp;
          is_clip=1;
        }
    }
  }
}

