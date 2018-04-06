#ifndef VOLUME_H
#define VOLUME_H

#include <stdint.h>
#include <socketlib.h>

#define MAX_VOL         20
#define VOL_STEP        2

void display_volume(float* volume_db, struct audio_packet* packet);

void mesure_volume(float* volume_db, struct audio_packet* packet);

void change_volume(uint8_t * audio, int audio_size);

#endif
