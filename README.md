# MINIOGG

A tiny, single-header, zero-allocation [Ogg](https://xiph.org/ogg/) muxer.

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

Then whenever you're ready to add a packet call `miniogg_add_packet()` and
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

## License

0BSD, see file `LICENSE`.
