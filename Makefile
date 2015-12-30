test: main fifo
	./main hf.conf

main: dongles.c main.c synchronize.c dongles.h synchronize.h correlate.c correlate.h configuration.c configuration.h
	gcc dongles.c main.c synchronize.c correlate.c configuration.c -o main -Wall -Wextra -O3 -lm -ffast-math -lrtlsdr -lpthread -lfftw3f

fifo:
	mkfifo fifo

