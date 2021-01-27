CFLAGS = -c -Wall
CC = gcc
LIBS =  -lm 

all: client server

client: client.c
	${CC} client.c -o client

server: server.c linklist.c
	${CC} server.c linklist.c -o server

.PHONY: clean
clean:
	rm -f *.o *.out

