all: fui
fui: fui.c
	gcc -Wall -Werror -std=c11 fui.c -o fui -lncurses
clean:
	rm fui
