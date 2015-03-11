CC=gcc
CFLAGS=-std=c99 -Wall -Werror -g

all: vid_cap

clean:
	rm vid_cap

vid_cap: v4l2_helper.c vid_cap.c
	$(CC) $(CFLAGS) $^ -o $@
