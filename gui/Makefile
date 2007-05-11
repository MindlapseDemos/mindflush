obj = cfg.o leitm.o
bin = guicfg

CC = gcc
CFLAGS = -g -std=c89 -pedantic -Wall
LDFLAGS = -L/usr/X11R6/lib -lXm -lXt -lX11

$(bin): $(obj)
	$(CC) $(CFLAGS) -o $@ $(obj) $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(obj) $(bin)
