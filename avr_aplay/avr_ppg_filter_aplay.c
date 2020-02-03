#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "../data/ppg_data.h"

/**
	\file avr_ppg_aplay.c
	\author Jacek Wieczorek

	\brief A proof-of-concept implementation of wavetable synthesis (based on PPG Wave) meant to be
	easily ported for AVR devices.

	All calculations are performed using variables no bigger than 16 bits. The file could still probably
	use some optimisations, but I'll leave that for later, when I actually get to work with the real hardware.

	For now, this program outputs 8-bit data meant for aplay on stdout. The sampling frequency is configured
	using SAMPLING_FREQ macro.

	I've also implemented two 1-pole filters chained together. They work pretty nicely and surely make the sound
	more sophisticated. For that, see the file avr_ppg_filter_aplay.c.

	There's still a lot of work to do - for example LFO and EGs.
*/

//! I think we can manage that...
#define SAMPLING_FREQ 20000

//! This would be 64, but we don't need the additional 3 waveforms that PPG provides
#define DEFAULT_WAVETABLE_SIZE 61

//! Contains currently used wavetable
static struct wavetable_entry
{
	const uint8_t *ptr_l;
	const uint8_t *ptr_r;
	uint8_t factor;
	uint8_t is_key;
} current_wavetable[DEFAULT_WAVETABLE_SIZE];

//! Returns a pointer to the wave with certain index (that can later be passed to get_waveform_sample())
static inline const uint8_t *get_waveform_pointer( uint8_t index )
{
	return ppg_waveforms + ( index << 6 );
}

//! This shall act as pgm_read_byte()
static inline uint8_t get_waveform_sample( const uint8_t *ptr, uint8_t sample )
{
	return ptr[sample];
}

//! Reads sample from a 64-byte waveform buffer based on 16-bit phase value
static inline uint8_t get_waveform_sample_by_phase( const uint8_t *ptr, uint16_t phase2b )
{
	// This phase ranges 0-127
	uint8_t phase = ((uint8_t*) &phase2b)[1] >> 1;
	uint8_t half_select = phase & 64;
	phase &= 63; // Poor man's modulo 64

	// Waveform mirroring
	if ( half_select )
		return get_waveform_sample( ptr, phase );
	else
		return 255u - get_waveform_sample( ptr, 63u - phase );
}

//! Reads a single sample based on a wavetable entry
static inline uint8_t get_wavetable_sample( const struct wavetable_entry *e, uint16_t phase2b )
{
	uint8_t sample_l = get_waveform_sample_by_phase( e->ptr_l, phase2b );
	uint8_t sample_r = get_waveform_sample_by_phase( e->ptr_r, phase2b );
	uint8_t factor = e->factor;
	uint16_t mix_l = ( 256 - factor ) * sample_l;
	uint16_t mix_r = factor * sample_r;
	uint16_t mix = mix_l + mix_r;
	return mix >> 8;
}

//! Reads a single sample from the global wavetable
static inline uint8_t get_current_wavetable_sample( uint8_t slot, uint16_t phase2b )
{
	return get_wavetable_sample( current_wavetable + slot, phase2b );
}

/**
	Load a wavetable stored in PPG Wave 2.2 format into an array of wavetable_entry structs of size wavetable_size
	Returns a pointer to the next wavetable
*/
const uint8_t *load_wavetable( struct wavetable_entry *entries, uint8_t wavetable_size, const uint8_t *data )
{
	// Wipe the wavetable
	memset( entries, 0, wavetable_size * sizeof( struct wavetable_entry ) );

	// The fist byte is ignored
	data++;

	// Read wavetable entries up to size - 1
	uint8_t waveform, pos;
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
	for ( uint8_t i = 0; i < wavetable_size; i++ )
	{
		// If the current entry contains a key-wave
		if ( entries[i].is_key )
		{
			// Write both pointers in case the right key waveform is not found
			el = er = &entries[i];

			// Look for the next key-wave
			for ( uint8_t j = i + 1; j < wavetable_size; j++ )
			{
				if ( entries[j].is_key )
				{
					er = &entries[j];
					break;
				}
			}
		}

		// Total distance between known key-waves and distance from the left one
		uint8_t distance_total = er - el;
		uint8_t distance_l = &entries[i] - el;

		entries[i].ptr_l = el->ptr_l;
		entries[i].ptr_r = er->ptr_l;

		// We have to avoid division by 0 for the last slot
		if ( distance_total != 0 )
			entries[i].factor = ( 65535 / distance_total * distance_l ) >> 8;
		else
			entries[i].factor = 0;
	}

	// Return pointer to the next wavetable
	return data;
}

//! Loads n-th requested wavetable from binary format
//! Not very efficient, but it doesn't need to be.
//! \see load_wavetable()
const uint8_t *load_wavetable_n( struct wavetable_entry *entries, uint8_t wavetable_size, const uint8_t *data, uint8_t index )
{
	for ( uint8_t i = 0; i < index + 1; i++ )
		data = load_wavetable( entries, wavetable_size, data );
	return data;
}

//! Safe add (no overflow and underflow)
static inline int16_t safe_add( int16_t a, int16_t b )
{
	if ( a > 0 && b > INT16_MAX - a )
		return INT16_MAX;
	else if ( a < 0 && b < INT16_MIN - a )
		return INT16_MIN;
	return a + b;
}

// A 16-bit overflow/underflow-safe digital integrator
typedef int16_t integrator;
static inline integrator integrator_feed( integrator *i, integrator x )
{
	return *i = safe_add( *i, x );
	// return *i += x;
}

//! A 1 pole filter based on the above integrator
//! \see integrator
typedef int8_t audio_signal;
typedef integrator filter1pole;
static inline audio_signal filter1pole_feed( filter1pole *f, int8_t k, audio_signal x )
{
	integrator_feed( f, ( x - ( *f / 256 ) ) * k );
	return *f / 256;
}

int main( int argc, char **argv )
{
	// Load wavetable
	load_wavetable_n( current_wavetable, DEFAULT_WAVETABLE_SIZE, ppg_wavetable, 18 );

	// The main loop
	while ( 1 )
	{
		// DDS
		static uint16_t phase = 0;
		static float f = 62;
		uint16_t phase_step = 65536 * f / SAMPLING_FREQ;

		// Time counter
		static uint32_t cnt = 0;
		static float t = 0;
		cnt++;
		t = (float)cnt / SAMPLING_FREQ;

		// Waveform generation
		uint8_t sample = get_current_wavetable_sample( 30 + 30 * sin( t ), phase  );

		// Two 1-pole filters chained together
		audio_signal x = sample - 127;
		int8_t k = 64 + sin( 32 * t ) * 30;
		static filter1pole Fa = 0, Fb = 0;
		audio_signal y = filter1pole_feed( &Fb, k, filter1pole_feed( &Fa, k, x ) );

		// Audio output and phase stepping
		putchar( 127 + y );
		phase += phase_step;
	}

	return 0;
}



