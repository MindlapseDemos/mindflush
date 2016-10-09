obj = cfg.o leitm.o
bin = guicfg

CC = gcc
CFLAGS = -m32 -g -std=c89 -pedantic -Wall
LDFLAGS = -L/usr/X11R6/lib -L/emul/ia32-linux/usr/lib -lXm -lXt -lX11

$(bin): $(obj)
	$(CC) $(CFLAGS) -o $@ $(obj) $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(obj) $(bin)
