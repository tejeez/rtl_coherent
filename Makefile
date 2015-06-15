test: main
	./main

main: dongles.c main.c
	gcc dongles.c main.c -o main -Wall -Wextra -O3 -lm -lrtlsdr -lpthread

