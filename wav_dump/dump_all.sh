#!/bin/bash
cd "$(dirname "$0")"

SAMPLERATE=8000

mkdir -p wav

for n in {0..28}; do
	./ppg_wt_dump $n 10 | ./mkwav "wav/long_${n}.wav" $SAMPLERATE
	./ppg_wt_dump $n 1 | ./mkwav "wav/${n}.wav" $SAMPLERATE
done;
