all:
	clang -o ppg_aplay -Wall ppg_aplay.c data/ppg_data.c -fsanitize=address -g -lm 

run: all
	./ppg_aplay | aplay -r 20000
