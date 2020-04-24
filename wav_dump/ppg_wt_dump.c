#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "../data/ppg_data.h"

/**
	\file ppg_wt_dump.c
	\author Jacek Wieczorek

	\brief Dumps selected wavetable to stdout.
	
*/

//! This would be 64, but we don't need the additional 3 waveforms that PPG provides
#define DEFAULT_WAVETABLE_SIZE 61

//! A wavetable entry/slot
struct wavetable_entry
{
	const uint8_t *ptr_l;
	const uint8_t *ptr_r;
	float factor;
	uint8_t is_key;
};

//! Contains currently used wavetable
static struct wavetable_entry current_wavetable[DEFAULT_WAVETABLE_SIZE];

//! Returns a pointer to the wave with certain index (that can later be passed to get_waveform_sample())
static inline const uint8_t *get_waveform_pointer( unsigned int index )
{
	return ppg_waveforms + index * 64;
}

//! Returns a sample (float) from a waveform
static inline float get_waveform_sample( const uint8_t *ptr, uint8_t sample )
{
	return ( ptr[sample] - 128 ) / 128.f;
}

//! Reaturns a float sample from waveform based on 0 - 1 phase value
static inline float get_waveform_sample_by_phase( const uint8_t *ptr, float phase )
{
	// phase [0; 0.5) ==> samples [0; 63)
	// phase [0.5; 1) ==> samples [63;0) (inverted)

	if ( phase < 0.5f )
		return get_waveform_sample( ptr, phase * 2 * 64 );                 
	else
		return -get_waveform_sample( ptr, 63 - ( phase - 0.5f ) * 2 * 64 );
		
}

//! Reads a single sample based on a wavetable entry
static inline float get_wavetable_sample( const struct wavetable_entry *e, float phase )
{
	float sample_l = get_waveform_sample_by_phase( e->ptr_l, phase );
	float sample_r = get_waveform_sample_by_phase( e->ptr_r, phase );
	float t = e->factor;

	// Perform linear interpolation
	return ( 1.f - t ) * sample_l + t * sample_r;
}

//! Reads a single sample from the global wavetable
static inline float get_current_wavetable_sample( unsigned int slot, float phase )
{
	return get_wavetable_sample( current_wavetable + slot, phase );
}

/**
	Load a wavetable stored in PPG Wave 2.2 format into an array of wavetable_entry structs of size wavetable_size
	Returns a pointer to the next wavetable
*/
const uint8_t *load_wavetable( struct wavetable_entry *entries, unsigned int wavetable_size, const uint8_t *data )
{
	// Wipe the wavetable
	memset( entries, 0, wavetable_size * sizeof( struct wavetable_entry ) );

	// The fist byte is ignored
	data++;

	// Read wavetable entries up to size - 1
	unsigned int waveform, pos;
	do
	{
		waveform = *data++;
		pos = *data++;

		entries[pos].ptr_l = get_waveform_pointer( waveform );
		entries[pos].ptr_r = NULL;
		entries[pos].factor = 0;
		entries[pos].is_key = 1;
	}
	while ( pos < wavetable_size - 1 );

	// Now, generate interpolation coefficients
	const struct wavetable_entry *el = NULL, *er = NULL;
	for ( unsigned int i = 0; i < wavetable_size; i++ )
	{
		// If the current entry contains a key-wave
		if ( entries[i].is_key )
		{
			// Write both pointers in case the right key waveform is not found
			el = er = &entries[i];

			// Look for the next key-wave
			for ( unsigned int j = i + 1; j < wavetable_size; j++ )
			{
				if ( entries[j].is_key )
				{
					er = &entries[j];
					break;
				}
			}
		}

		// Total distance between known key-waves and distance from the left one
		int distance_total = er - el;
		int distance_l = &entries[i] - el;

		entries[i].ptr_l = el->ptr_l;
		entries[i].ptr_r = er->ptr_l;

		// We have to avoid division by 0 for the last slot
		entries[i].factor = distance_total ? (float) distance_l / distance_total : 0.0f;
	}

	// Return pointer to the next wavetable
	return data;
}

//! Loads n-th requested wavetable from binary format.
//! Not very efficient, but it doesn't need to be.
//! \see load_wavetable()
const uint8_t *load_wavetable_n( struct wavetable_entry *entries, unsigned int wavetable_size, const uint8_t *data, unsigned int index )
{
	for ( unsigned int i = 0; i < index + 1; i++ )
		data = load_wavetable( entries, wavetable_size, data );
	return data;
}




int main( int argc, char **argv )
{
	if (argc < 3)
	{
		fprintf(stderr, "Usage: %s <WAVETABLE INDEX> <REPEAT>\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	
	
	// Wavetable index
	int wt_index;
	if (sscanf(argv[1], "%d", &wt_index) != 1 || wt_index < 0)
	{
		fprintf(stderr, "invalid wavetable index\n");
		exit(EXIT_FAILURE);
	}
	
	// How many times should each waveform be repeated
	// before switching to the next one
	int repeat;
	if (sscanf(argv[2], "%d", &repeat) != 1 || repeat <= 0)
	{
		fprintf(stderr, "invalid repeat value\n");
		exit(EXIT_FAILURE);
	}
	
	// Load wavetable
	load_wavetable_n(current_wavetable, DEFAULT_WAVETABLE_SIZE, ppg_wavetable, wt_index);

	// For each slot
	for (int slot = 0; slot < DEFAULT_WAVETABLE_SIZE; slot++)
	{
		// Reapeat each waveform
		for (int i = 0; i < repeat; i++)
		{
			// Dump waveform
			for (int phase = 0; phase < 128; phase++)
			{
				float sample = get_current_wavetable_sample(slot, phase / 128.f);
				putchar( 128 + sample * 127.f );
			}
		}
	}

	return 0;
}



