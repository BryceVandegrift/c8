# c8 version
VERSION = 0.1

CC = cc
INC = -I/usr/include/SDL2
CPPFLAGS = -DVERSION=\"$(VERSION)\"
CFLAGS = -std=c99 -pedantic -Wall -Os ${INC} ${CPPFLAGS}
LDFLAGS = -lSDL2

SRC = c8.c sdl.c util.c
OBJ = ${SRC:.c=.o}

all: c8

.c.o:
	${CC} -c ${CFLAGS} $<

c8: ${OBJ}
	${CC} -o $@ ${OBJ} ${LDFLAGS}

c8-static: ${OBJ}
	${CC} -o $@ ${OBJ} ${LDFLAGS} -static

test: c8
	./c8 -r tests/chip8-logo.ch8
	./c8 -r tests/ibm-logo.ch8
	./c8 -r tests/corax.ch8
	./c8 -r tests/flags.ch8
	./c8 -r tests/quirks.ch8
	./c8 -r tests/keypad.ch8
	./c8 -r tests/beep.ch8

clean:
	rm -f c8 c8-static ${OBJ}

.PHONY: all clean
