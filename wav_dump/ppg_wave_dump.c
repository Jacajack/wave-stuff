#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>

#include "../data/ppg_data.h"

/**
	\file ppg_wt_dump.c
	\author Jacek Wieczorek

	\brief Dumps selected waveform.
	
*/

uint8_t get_waveform_sample(uint8_t waveform_id, uint8_t phase)
{
	assert(phase < 128);
	return phase < 64 ? ppg_waveforms[waveform_id * 64 + phase]
		              : ~ppg_waveforms[waveform_id * 64 + 63 - (phase - 64)];
}

int main(int argc, char **argv)
{
	if (argc < 3)
	{
		fprintf(stderr, "Usage: %s <WAVE INDEX> <REPEAT>\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	
	int wave_index;
	if (sscanf(argv[1], "%d", &wave_index) != 1 || wave_index < 0 || wave_index > 255)
	{
		fprintf(stderr, "invalid wavetable index\n");
		exit(EXIT_FAILURE);
	}
	
	int repeat;
	if (sscanf(argv[2], "%d", &repeat) != 1 || repeat <= 0)
	{
		fprintf(stderr, "invalid repeat value\n");
		exit(EXIT_FAILURE);
	}
	
	for (int i = 0; i < repeat; i++)
	{
		for (int sample = 0; sample < 128; sample++)
		{
			putchar(get_waveform_sample(wave_index, sample));
		}
	}

	return 0;
}



