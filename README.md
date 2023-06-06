# MINIOGG

A tiny, single-header, zero-allocation [Ogg](https://xiph.org/ogg/) muxer/demuxer.

## Usage

It's pretty simple to use. In one C file, define `MINIOGG_IMPLEMENTATION`
and include the header file:

```c
#define MINIOGG_IMPLEMENTATION
#include "miniogg.h"
```

Then somewhere in your program, allocate a `miniogg` struct
and initialize it with a serial number:

```c
miniogg muxer;
miniogg_init(&muxer,1234);
```

If you're planning to demux, you can use any serial number,
it will be ignored anyway.

### Muxing

To add a packet call `miniogg_add_packet()` and
check the return value. If `miniogg_add_packet()` returns 0, the packet
was added to the Ogg page entirely. If it returns 1, the packet was
partially added - you should call `miniogg_finish_page()` to get the page
ready for writing and write it out immediately before calling `miniogg_add_packet()`
again.

```c
FILE* out = ... /* assuming we plan to write to a FILE* */
uint8_t *buffer = ... /* get data from wherever */
size_t buffer_len = ...
uint64_t granulepos = ...

/* used to track where we are within the buffer while adding the packet */
size_t used = 0;
size_t pos = 0;

while(miniogg_add_packet(&muxer,&buffer[pos],buffer_len,granuelpos,&used)) {
    miniogg_finish_page(&muxer);
    fwrite(muxer.header,1,muxer.header_len,out);
    fwrite(muxer.body,1,muxer.body_len,out);

    buffer_len -= used;
    pos += used;
}
```

Once you're done adding packets to your page, call `miniogg_page_finish()`
to encode the page headers, then write out the contents of the
`header` and `body` fields:

```c
miniogg_finish_page(&muxer);
fwrite(muxer.header,1,muxer.header_len,out);
fwrite(muxer.body,1,muxer.body_len,out);
```

When you're complete with writing your ogg stream, call `miniogg_eos()`
and write out the final page:

```c
miniogg_eos(&muxer);
fwrite(muxer.header,1,muxer.header_len,out);
fwrite(muxer.body,1,muxer.body_len,out);
```

One thing to note - this library has no concept of auto-flushing a page.
As far as I know, all the various (x)-in Ogg mappings require header packets
to be on their own page(s). Be sure to check your codec's Ogg Mapping carefully.

Additionally, `miniogg_add_packet` will record the amount of bytes used
into the `used` outvar. It's entirely possible to consume all the bytes
of the packet data, but still return 1 because the muxer needs to write
a final terminating byte on the next page. You should not assume that
writing all the bytes of the packet means you're complete, only the return
value of `miniogg_add_packet` can be used for that.

### Demuxing

To add a page call `miniogg_add_page()` and
check the return value. If `miniogg_add_page()` returns 0, the full Ogg page
has been loaded and parsed, and you can start calling `miniogg_get_packet()`
or `miniogg_iter_packet()`.
If it returns 1, then more data is needed. Anything else is an error.

Similar to muxing with `miniogg_add_packet()` -  `miniogg_add_page()` has an
outvar - `used` - that returns the number of bytes read from your data
stream.


```c
#define BUFFER_SIZE 4096
uint8_t buffer[BUFFER_SIZE];
FILE* in = ... /* assuming we plan to read from FILE* */

size_t len = 0; /* will store number of bytes read from FILE */
size_t used = 0; /* number of bytes consumed by miniogg_add_page */
size_t pos = 0; /* current position within the buffer */

miniogg demuxer;
miniogg_init(&demuxer,0);

while( (len = fread(buffer,1,BUFFER_SIZE,in)) > 0) {
    pos = 0;
    while(miniogg_add_page(&demuxer,&buffer[pos],len,&used) == 0) {
        /* do things with the page */
        pos += used;
        len -= used;
    }
}
```

Once `miniogg_add_page()` returns 0, all the struct fields will be loaded
and can be inspected.

To get packets, call `miniogg_iter_packet()`. You should repeatedly
call this function until it returns `NULL`.

```c
const uint8_t *packet = NULL;
size_t packet_len = 0;
uint64_t granulepos = 0;
uint8_t continued = 0;

while( (packet = miniogg_iter_packet(&demuxer,&packet_len,&granulepos,&continued)) != NULL) {
    /* do something with packet */
}
```

Alternatively, if you need to retrieve an individual packet out-of-order,
you can use `miniogg_get_packet()`. This is less efficient, as looking
up out-of-order packets requires re-calculating byte offsets and re-scanning
the segment table. Similar to `miniogg_iter_packet()` - call repeatedly
with an increasing packet number variable until it returns `NULL`.

Note the `packets` struct field should *not* be used for determining
how many packets to retrieve, that variable only marks the amount
of packets that complete on the page. A packet may continue on to
the next page, in which case the `cont` outvar will be set to 1. You
should buffer the data and concatenate it with the next call to
`miniogg_iter_packet()` or `miniogg_get_packet()`.

It's entirely valid for the returned packet size to be `0`, this
happens when a packet's size is perfectly aligned to a page size,
and requires a final terminating segment. This should only occur
when you're dealing with a packet that spans multiple pages.

## `miniogg` function reference:

```c
/* resets all fields to default values and sets a serial number */
MINIOGG_API
void miniogg_init(miniogg* p, uint32_t serialno);

/* returns 0 if packet was added fully, 1 if continuation is needed,
 * the number of bytes read is returned in used */
MINIOGG_API
int miniogg_add_packet(miniogg* p, const void* data, size_t len, uint64_t granulepos, size_t *used);

/* encodes flags, granulepos, etc, calculates the crc32,
 * sets the header_len and body_len fields, increases pageno
 * and resets the bos/eos/continuation flags */
MINIOGG_API
void miniogg_finish_page(miniogg* p);

/* similar to miniogg_finish_page but sets the end-of-stream flag to true */
MINIOGG_API
void miniogg_eos(miniogg* p);

/* returns how large the ogg page would be if
 * written out right now (header length + body length) */
MINIOGG_API
uint32_t miniogg_used_space(const miniogg* p);

/* returns how much space is available for data in the current page,
 * without having to continue into another page. */
MINIOGG_API
uint32_t miniogg_available_space(const miniogg* p);

/* returns 0 if the page was added fully, 1 if more bytes are needed,
 * the number of bytes read is returned in used */
MINIOGG_API
int miniogg_add_page(miniogg* p, const void* data, size_t len, size_t *used);

/* returns a pointer to the start of packet data, the length of the packet is stored
 * in len. uses an internal counter to track packet. If the packet is
 * continued on another page, sets cont to 1, 0 otherwise */
MINIOGG_API
const void* miniogg_iter_packet(miniogg* p, size_t *len, uint64_t *granulepos, uint8_t *cont);

/* returns a pointer to the start of packet data, the length of the packet is stored
 * in len. returns NULL if the packetno is too high. If the packet is
 * continued on another page, sets cont to 1, 0 otherwise */
MINIOGG_API
const void* miniogg_get_packet(const miniogg* p, uint32_t packetno, size_t *len, uint64_t *granulepos, uint8_t *cont);
```

## `miniogg` struct fields

The following fields can be set by the user sometime before
calling `miniogg_page_finish()`:

* `uint32_t serialno` - every ogg bitstream should have a unique serial number,
`miniogg_init()` sets this.
* `uint8_t eos` - a flag indicating the page is the end of the stream,
`miniogg_eos()` sets this.

These fields can be read after calling `miniogg_add_packet()`:

* `uint64_t granulepos` - the granule position of the last added packet,
or `0xffffffffffffffff` if the page has a single packet spanning the entire page.
* `uint32_t segment` - the number of page segments used (between 1-255).
* `uint32_t packets` - the number of packets that end on this page.

These fields are meant to be read before calling `miniogg_page_finish()`:

* `uint32_t bos` - a flag indicating this is the beginning of the stream.
* `uint32_t pageno` - the current page number.
* `uint8_t continuation` - a flag that, if non-zero, indicates this page
continues a packet from the previous page.

The following fields are meant to be read/used after calling `miniogg_page_finish()`:

* `uint8_t header[282]` - contains the bytes of the Ogg page headers.
* `uint32_t header_len` - contains the length of Ogg page headers.
* `uint8_t body[65025]` - contains the bytes of the Ogg page body.
* `uint32_t body_len` - contains the length of Ogg page body.

The rest are intended to be read to inspect the state of the current page,
or to get the page header and body after calling `miniogg_page_finish()` or
`miniogg_eos()`.

When demuxing, all fields are intended to be readable by the user.

## License

0BSD, see file `LICENSE`.
