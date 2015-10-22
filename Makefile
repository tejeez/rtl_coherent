test: main
	./main

main: dongles.c main.c synchronize.c dongles.h synchronize.h correlate.c correlate.h
	gcc dongles.c main.c synchronize.c correlate.c -o main -Wall -Wextra -O3 -lm -ffast-math -lrtlsdr -lpthread -lfftw3f

