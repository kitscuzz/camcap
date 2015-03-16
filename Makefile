CC=gcc
CFLAGS=-std=c99 -Wall -Werror -g

all: camcap

clean:
	rm camcap

camcap: v4l2_helper.c camcap.c
	$(CC) $(CFLAGS) $^ -o $@
