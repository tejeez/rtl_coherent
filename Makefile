test: main
	./main

main: dongles.c main.c synchronize.c
	gcc dongles.c main.c synchronize.c -o main -Wall -Wextra -O3 -lm -lrtlsdr -lpthread -lfftw3f

