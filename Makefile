CFLAGS = -Wall -Wextra -std=c11 -g -Iglad/include $(shell pkg-config --cflags sdl3)
LDLIBS = $(shell pkg-config --libs sdl3) -ldl

pong: main.c glad/src/glad.c
	gcc $(CFLAGS) main.c glad/src/glad.c -o pong.elf $(LDLIBS)

clean:
	rm -f pong.elf