all: fui
fui: main.c
	gcc -Wall -Werror -std=c11 main.c -o fui -lncurses
clean:
	rm fui
