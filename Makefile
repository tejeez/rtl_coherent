test: main
	./main

main: dongles.c main.c synchronize.c dongles.h synchronize.h
	gcc dongles.c main.c synchronize.c -o main -Wall -Wextra -O3 -lm -lrtlsdr -lpthread -lfftw3f

