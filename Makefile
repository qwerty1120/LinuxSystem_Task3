CC=gcc
OPTIONS= -g -lcrypto

EXECUTABLES = add remove help ssu_sync list

all: $(EXECUTABLES)

sync_header.o : sync_header.c sync_header.h
	$(CC) -c sync_header.c $(OPTIONS)

$(EXECUTABLES): %: %.c sync_header.o
	$(CC) -o $@ $^ $(OPTIONS)

clean:
	rm -rf *.o $(EXECUTABLES) .repo