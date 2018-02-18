CC=$(CROSS_COMPILE)gcc

CFLAGS+=-Wall -std=gnu11 -fPIC
LDFLAGS+=-lconfuse

# VERSION
MAJOR=0
MINOR=0
PATCH=0
VERSION = -DVERSION_MAJOR=$(MAJOR) -DVERSION_MINOR=$(MINOR) -DVERSION_PATCH=$(PATCH)

INCLUDE += -Isrc/

.PHONY: all

all: gpiod

.PHONY: debug

debug: CFLAGS += -O0 -DDEBUG -g -Wno-unused-variable
debug: all

UAPI_OBJECTS=

ifndef WITHOUT_GPIOD_UAPI
UAPI_OBJECTS=gpiod-uapi.o gpiod-chip-uapi.o
else
CFLAGS+=-DWITHOUT_GPIOD_UAPI=1
endif

gpiod: gpiod.o  gpiod-config.o  gpiod-server.o gpiod-event-loop.o gpiod-dispatch.o gpiod-dispatch-raw.o gpiod-exec.o gpiod-chip.o gpiod-chip-sysfs.o gpiod-pin.o gpiod-sysfs.o $(UAPI_OBJECTS)
	$(CC) $(CFLAGS) -o gpiod gpiod.o gpiod-config.o gpiod-server.o gpiod-event-loop.o gpiod-dispatch.o gpiod-dispatch-raw.o gpiod-exec.o gpiod-chip.o gpiod-chip-sysfs.o gpiod-pin.o gpiod-sysfs.o $(UAPI_OBJECTS) $(LDFLAGS)

gpiod.o: src/gpiod.c
	$(CC) $(CFLAGS) -c src/gpiod.c $(INCLUDE)

gpiod-config.o: src/gpiod-config.c
	$(CC) $(CFLAGS) -c src/gpiod-config.c $(INCLUDE)

gpiod-server.o: src/gpiod-server.c
	$(CC) $(CFLAGS) -c src/gpiod-server.c $(INCLUDE)

gpiod-event-loop.o: src/gpiod-event-loop.c
	$(CC) $(CFLAGS) -c src/gpiod-event-loop.c $(INCLUDE)

gpiod-dispatch.o: src/gpiod-dispatch.c
	$(CC) $(CFLAGS) -c src/gpiod-dispatch.c $(INCLUDE)

gpiod-dispatch-raw.o: src/dispatch/gpiod-dispatch-raw.c
	$(CC) $(CFLAGS) -c src/dispatch/gpiod-dispatch-raw.c $(INCLUDE)

gpiod-exec.o: src/gpiod-exec.c
	$(CC) $(CFLAGS) -c src/gpiod-exec.c $(INCLUDE)

gpiod-chip.o: src/gpiod-chip.c
	$(CC) $(CFLAGS) -c src/gpiod-chip.c $(INCLUDE)

gpiod-pin.o: src/gpiod-pin.c
	$(CC) $(CFLAGS) -c src/gpiod-pin.c $(INCLUDE)

gpiod-sysfs.o: src/pins/gpiod-sysfs.c
	$(CC) $(CFLAGS) -c src/pins/gpiod-sysfs.c $(INCLUDE)

gpiod-chip-sysfs.o: src/chips/gpiod-chip-sysfs.c
	$(CC) $(CFLAGS) -c src/chips/gpiod-chip-sysfs.c $(INCLUDE)

ifndef WITHOUT_GPIOD_UAPI
gpiod-uapi.o: src/pins/gpiod-uapi.c
	$(CC) $(CFLAGS) -c src/pins/gpiod-uapi.c $(INCLUDE)

gpiod-chip-uapi.o : src/chips/gpiod-chip-uapi.c
	$(CC) $(CFLAGS) -c src/chips/gpiod-chip-uapi.c $(INCLUDE)
endif

DESTDIR=
prefix = /usr/local
exec_prefix = $(DESTDIR)$(prefix)
sbindir = $(exec_prefix)/sbin

.PHONY: install uninstall
install: all
	install -v -m 0755 gpiod $(sbindir)/gpiod
	mkdir -p $(DESTDIR)/etc/gpiod
	install -v -m 0644 etc/gpiod.conf $(DESTDIR)/etc/gpiod

uninstall:
	-rm $(sbindir)/gpiod
	-rm $(DESTDIR)/etc/gpiod/gpiod.conf

clean:
	-rm gpiod *.o
