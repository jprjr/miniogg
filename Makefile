.PHONY: all clean

LIBOGG_CFLAGS = $(shell pkg-config --cflags ogg)
LIBOGG_LDFLAGS = $(shell pkg-config --libs ogg)
CFLAGS = -Wall -Wextra -Wconversion -Wdouble-promotion -Wno-sign-conversion -g

all: demos/repack miniogg.o

miniogg.o: miniogg.c miniogg.h
	$(CC) $(CFLAGS) -o $@ -c $<

demos/repack: demos/repack.o
	$(CC) -o $@ $^ $(LIBOGG_LDFLAGS) $(LDFLAGS)

demos/repack.o: demos/repack.c miniogg.h
	$(CC) $(CFLAGS) $(LIBOGG_CFLAGS) -o $@ -c $<

clean:
	rm -f miniogg.o demos/repack demos/repack.o
