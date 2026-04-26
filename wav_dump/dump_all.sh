#!/bin/bash
cd "$(dirname "$0")"

SAMPLERATE=8000

mkdir -p wav

for n in {0..28}; do
	./ppg_wt_dump 61 $n 10 | ./mkwav "wavetables/long_${n}.wav" $SAMPLERATE
	./ppg_wt_dump 61 $n 1 | ./mkwav "wavetables/${n}.wav" $SAMPLERATE
	./ppg_wt_dump 128 $n 1 | ./mkwav "wavetables/hires_${n}.wav" $SAMPLERATE
	./ppg_wt_dump 128 $n 10 | ./mkwav "wavetables/long_hires_${n}.wav" $SAMPLERATE
done;

for n in {0..255}; do
	./ppg_wave_dump $n 100 | ./mkwav "waveforms/wave_long_${n}.wav" $SAMPLERATE
	./ppg_wave_dump $n 1 | ./mkwav "waveforms/wave_${n}.wav" $SAMPLERATE
done;

