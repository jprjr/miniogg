.PHONY: all clean

LIBOGG_CFLAGS = $(shell pkg-config --cflags ogg)
LIBOGG_LDFLAGS = $(shell pkg-config --libs ogg)
CFLAGS = -Wall -Wextra -Wconversion -Wdouble-promotion -Wno-sign-conversion -g

all: demos/repack demos/list-packets-miniogg demos/list-packets-libogg miniogg.o

miniogg.o: miniogg.c miniogg.h
	$(CC) $(CFLAGS) -o $@ -c $<

demos/repack: demos/repack.o
	$(CC) -o $@ $^ $(LIBOGG_LDFLAGS) $(LDFLAGS)

demos/repack.o: demos/repack.c miniogg.h
	$(CC) $(CFLAGS) $(LIBOGG_CFLAGS) -o $@ -c $<

demos/list-packets-miniogg: demos/list-packets-miniogg.o
	$(CC) -o $@ $^ $(LDFLAGS)

demos/list-packets-miniogg.o: demos/list-packets-miniogg.c miniogg.h
	$(CC) $(CFLAGS) -o $@ -c $<

demos/list-packets-libogg: demos/list-packets-libogg.o
	$(CC) -o $@ $^ $(LIBOGG_LDFLAGS) $(LDFLAGS)

demos/list-packets-libogg.o: demos/list-packets-libogg.c
	$(CC) $(CFLAGS) $(LIBOGG_CFLAGS) -o $@ -c $<

clean:
	rm -f miniogg.o
	rm -f demos/repack demos/repack.o demos/repack.exe
	rm -f demos/list-packets-miniogg demos/list-packets-miniogg.o demos/list-packets-miniogg.exe
	rm -f demos/list-packets-libogg demos/list-packets-libogg.o demos/list-packets-libogg.exe
