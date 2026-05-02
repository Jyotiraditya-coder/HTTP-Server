CC = gcc
CFLAGS = -Wall -Wextra
 
all: httpserver
 
httpserver: httpserver.c
	$(CC) $(CFLAGS) httpserver.c -o httpserver
 
run: httpserver
	./httpserver
 
clean:
	rm -f httpserver
