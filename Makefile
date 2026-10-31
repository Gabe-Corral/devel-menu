CC ?= cc
PKGS = gtk+-3.0 gtk-layer-shell-0 gio-unix-2.0
CFLAGS += -std=c11 -O2 -Wall -Wextra -Wpedantic $(shell pkg-config --cflags $(PKGS))
LDLIBS += $(shell pkg-config --libs $(PKGS))

all: devel-menu

devel-menu: src/main.c src/apps.c src/apps.h
	$(CC) $(CFLAGS) src/main.c src/apps.c -o $@ $(LDLIBS)

test: devel-menu
	$(CC) $(CFLAGS) tests/apps.c src/apps.c -o tests/apps-test $(LDLIBS)
	./tests/apps-test

clean:
	rm -f devel-menu tests/apps-test

.PHONY: all test clean
