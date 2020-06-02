PROGS = client server

CFLAGS = -g -Wall

all: client server

client: client.c
	gcc ${CFLAGS} -o client client.c parser.c

server: server.c
	gcc ${CFLAGS} -o server server.c parser.c

clean:
	rm -rf ${PROGS}