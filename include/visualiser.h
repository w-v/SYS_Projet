#ifndef VISUALISER_H
#define VISUALISER_H

#include <stdint.h>
#include <socketlib.h>

#define WIN_SIZE        3
#define LOW_FREQ        25

void visualize_window(uint8_t * crt_packet, uint8_t window[WIN_SIZE][BUF_SIZE], int packet_id);

void visualize(uint8_t * audio, int audio_size);

void log_scale(double * unscaled, int size_unscaled, double * scaled, int size_scaled);
#endif
