#include <visualiser.h>
#include <fftw3.h>
#include <guestutils.h>
#include <smplutils.h>
#include <ncurses.h>
#include <math.h>
#include <complex.h>
#include <socketlib.h>



void visualize_window(uint8_t * crt_packet, uint8_t window[WIN_SIZE][BUF_SIZE], int packet_id){

  // copy current packet at the end of the window
  memcpy(&window[WIN_SIZE - 1], crt_packet, sizeof(window[0]));
  if(packet_id > 1)
    visualize((uint8_t *) window+BUF_SIZE*1, BUF_SIZE*(WIN_SIZE-1));

  // shift window to the right (packets towards the start of the array)
  for(int p = WIN_SIZE-1; p > 0; p--){
    memcpy(&window[p-1], &window[p], sizeof(window[0]));        // nasty stuff
  }

}

void visualize(uint8_t * audio, int audio_size){
  int nchans = params.channels;
  // number of samples per channel
  int nsmpls = audio_size / (params.sample_size/8) / params.channels;
  double window;
  int w, h;
  getmaxyx(stdscr, h, w);
  //w = 30;
  w -= 3;
  h-= 3;
  double * fft_bins = (double *) malloc(sizeof(double)*w);
  double ** fft_in = (double **) malloc(nchans*sizeof(double*));
  for (int ch = 0; ch < params.channels ; ch++){
    fft_in[ch] = fftw_alloc_real(nsmpls);
  }
  char_to_float(audio, audio_size, fft_in);
  for(int ch = 0; ch < nchans; ch++){
    for (int s = 0; s < nsmpls; s++){
      // Hann window (avoiding errors caused by the buffer being limited in size)
      window = 0.5f*( 1-cos( (2 * M_PI * s)/(nsmpls - 1) ) );
      fft_in[ch][s] *= window;
    }
  }
  fftw_complex * fft_out;
  fftw_plan plan;
  for (int i = 0; i < nchans; i++){
    fft_out = fftw_alloc_complex(nsmpls);
    for (int j = 0; j < (nsmpls); j++){
      fft_out[j] = 0;
    }
    plan = fftw_plan_dft_r2c_1d(nsmpls, fft_in[i], fft_out, 0); 
    fftw_execute(plan);
    for (int j = 0; j < nsmpls; j++){
      // complex absolute value calculation
      // fft_in becomes the output
      fft_in[i][j] = cabs(fft_out[j]);
    }
    
  }
  fftw_free(fft_out);
  
  // max between channels
  for (int j = 0; j < nsmpls /2; j++){
    if(fft_in[0][j] < fft_in[1][j]){
      fft_in[0][j] = fft_in[1][j];
    }
  }
  
  log_scale(fft_in[0], nsmpls/2, fft_bins, w);
  clear();
  
  for(int i = 0; i < w-2; i++){
    
    //fft_bins[i] = 20 * log10(fft_in[0][i]);
    // average BUF_SIZE bins into w bars
    /*for(int a = 0; a < bin_per_bar; a++){
      bar_h += fft_bins[i*bin_per_bar+a] / bin_per_bar;
    } */
    
    int peak = h-(fft_bins[i]-30.f)*(-1.f)*(h/80.f);
    for(int r = 0; r < peak;r++ ){
      
      mvaddch(h-r,i+4, 'W');

    }

  }
  for (int ch = 0; ch < params.channels ; ch++){
    fftw_free(fft_in[ch]);
  }
  free(fft_in);
  free(fft_bins);
  refresh();
  
}

void log_scale(double * unscaled, int size_unscaled, double * scaled, int size_scaled){
  // frequency covered by one unscaled bin
  double bin_range = params.sample_rate / 2 / (double) size_unscaled;
  // constant for mapping frequency range (LOW_FREQHz to Nyquist freq.) into window
  double c = (params.sample_rate/2/LOW_FREQ);
  int s = 0;
  double tg;
  double tgg;
  scaled[0] = 0;
  for(int u = 0; u < size_unscaled; u++){
    tg = LOW_FREQ*pow(c, (s/(double)size_scaled));
    tgg = bin_range*u;
    if( tgg > tg ){
      if(scaled[s] != 0)
        scaled[s] = 20 * log10(scaled[s]);
      s++;
      if(s >= size_scaled)
        return;
      scaled[s] = 0;
    }
    scaled[s] += unscaled[u];
    
  }
  
}
