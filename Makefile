CC = cc
CFLAGS = -std=c99 -pedantic -Wall -I/usr/include/SDL2 -Os
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

clean:
	rm -f c8 c8-static ${OBJ}

.PHONY: all clean
