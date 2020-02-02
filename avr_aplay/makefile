all:
	clang -o avr_ppg_aplay -Wall avr_ppg_filter_aplay.c ../data/ppg_data.c -fsanitize=address -g -lm 

run: all
	./avr_ppg_aplay | aplay -r 20000
