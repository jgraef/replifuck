CFLAGS = `pkg-config glib-2.0 gsl --cflags` -I include/ -Wall
LDFLAGS = `pkg-config glib-2.0 gsl --libs` -lncurses

.PHONY: all clean


all: replifuck

clean:
	rm replifuck


replifuck: main.c replifuck.c tui.c
	$(CC) -o $@ $(CFLAGS) $^ $(LDFLAGS)


main.c: include/replifuck.h include/tui.h
replifuck.c: include/replifuck.h
tui.c: include/replifuck.h include/tui.h

