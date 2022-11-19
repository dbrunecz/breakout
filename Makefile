#apt-get install libx11-dev

CFLAGS+= -Wall -Werror
CFLAGS+= -ggdb
LDFLAGS+= -L/usr/X11R6/lib
LDLIBS+= -lX11 -lm

breakout: breakout.o
	gcc $(LDFLAGS) $^ $(LDLIBS) -o $@

clean:
	@-rm -f breakout *.o 2&>/dev/null
