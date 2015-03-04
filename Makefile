CC=gcc
CFLAGS=-std=c99 -Wall -Werror -g

clean:
	rm vid_cap

all: vid_cap

vid_cap: v4l2_helper.c vid_cap.c
	$(CC) $(CFLAGS) $^ -o $@
