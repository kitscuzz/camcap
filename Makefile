CC=gcc
CFLAGS=-std=gnu99 -Wall -Wextra -Werror -g -O2

all: camcap

clean:
	rm camcap

camcap: v4l2_helper.c camcap.c
	$(CC) $(CFLAGS) $^ -o $@
