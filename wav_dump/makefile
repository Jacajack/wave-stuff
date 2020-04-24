all:
	gcc -o ppg_wt_dump -Wall ppg_wt_dump.c ../data/ppg_data.c -lm
	gcc -o mkwav -Wall mkwav.c -lm

run: all
	bash dump_all.sh
