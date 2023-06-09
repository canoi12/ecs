CC = gcc

%: examples/%.c
	$(CC) $< -o $@ -I. -lSDL2 