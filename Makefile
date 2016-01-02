
all: main sdl_waterfall fifo


test: all
	./run.sh


main: dongles.c main.c synchronize.c dongles.h synchronize.h correlate.c correlate.h configuration.c configuration.h df.c df.h
	gcc dongles.c main.c synchronize.c correlate.c configuration.c df.c -o main -Wall -Wextra -O3 -lm -ffast-math -lrtlsdr -lpthread -lfftw3f


sdl_waterfall: sdl_waterfall.c
	gcc sdl_waterfall.c -o sdl_waterfall -Wall -Wextra -O3 -lSDL -lm


fifo:
	mkfifo fifo

