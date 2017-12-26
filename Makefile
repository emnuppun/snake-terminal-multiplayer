
TARGETS = server client
CC = gcc
CFLAGS = -g

all: $(TARGETS)

server: server.c game.c queue.c
	$(CC) $(CFLAGS) -o server server.c game.c queue.c

client: client.c
	$(CC) $(CFLAGS) -o client client.c

clean:
	-rm -f $(TARGETS)
