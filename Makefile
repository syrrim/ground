SOURCES=nodes.c server.c parse.c map/src/map.c
INCLUDES=-Imap/src/ -I.
CCFLAGS=-Wall -std=c99 -g
BINARY=nodes
all:
	gcc ${SOURCES} -o ${BINARY} ${INCLUDES} ${CCFLAGS}

clean:
	rm ${BINARY}
