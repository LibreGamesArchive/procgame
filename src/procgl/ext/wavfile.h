#ifndef WAVFILE_H
#define WAVFILE_H

#include <stdio.h>
#include <stdint.h>

FILE* wavfile_open(const char* filename);
void wavfile_write(FILE* file, int16_t* data, int length);
void wavfile_close(FILE* file);

#define WAVFILE_SAMPLES_PER_SECOND 44100

#endif
