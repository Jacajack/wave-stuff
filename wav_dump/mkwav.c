#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <math.h>


/*
	Simple utility that reads 8-bit binary data from stdin and outputs it to a mono 16-bit WAV file.
	The sample rate can be adjusted with command line parameter.
*/

int w32(FILE *f, uint32_t w)
{
	return fwrite(&w, sizeof(w), 1, f);
}

int w16(FILE *f, uint16_t w)
{
	return fwrite(&w, sizeof(w), 1, f);
}

int main(int argc, char *argv[])
{
	if (argc < 3)
	{
		fprintf(stderr, "Usage: %s <OUTPUT FILE> <SAMPLERATE>\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	
	// Samplerate
	int samplerate;
	if (sscanf(argv[2], "%d", &samplerate) != 1 || samplerate <= 0)
	{
		fprintf(stderr, "invalid samplerate!\n");
		exit(EXIT_FAILURE);
	}
	
	// Open the output file
	FILE *f = fopen(argv[1], "wb");
	if (!f)
	{
		perror("could not open the output file");
		exit(EXIT_FAILURE);
	}
	
	// Some settings
	int channels = 1;
	int bits_per_sample = 16;
	
	// Output the header
	fprintf(f, "RIFF");
	long pos1 = ftell(f);
	fprintf(f, "????"); // this has to be replaced once we know the length
	fprintf(f, "WAVE");
	
	// Format chunk
	fprintf(f, "fmt ");
	w32(f, 16);
	w16(f, 1);          // audio format - PCM
	w16(f, channels);   // number of channels
	w32(f, samplerate); // samplerate
	w32(f, samplerate * bits_per_sample / 8); // byterate
	w16(f, 2);          // block align
	w16(f, bits_per_sample);
	
	// Data chunk
	fprintf(f, "data");
	long pos2 = ftell(f);
	fprintf(f, "????"); // this has to updated as well
	
	// Dump the data
	long sample_count = 0;
	for (int b; (b = getchar()) != EOF; sample_count++)
		w16(f, (b - 128) * 256);
	
	// Update the header
	fseek(f, pos1, SEEK_SET);
	w32(f, 36 + channels * bits_per_sample / 8 * sample_count);
	fseek(f, pos2, SEEK_SET);
	w32(f, channels * bits_per_sample / 8 * sample_count);
	
	// Close and exit
	fclose(f);
	return 0;
}
