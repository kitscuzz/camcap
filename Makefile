CC=gcc
CFLAGS=-std=c99 -Wall -Werror

all: vid_cap

vid_cap: vid_cap.c
	$(CC) $(CFLAGS) $< -o $@
