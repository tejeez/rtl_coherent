test: dongles
	./dongles

dongles: dongles.c
	gcc dongles.c -o dongles -Wall -Wextra -O3 -lm -lrtlsdr -lpthread
