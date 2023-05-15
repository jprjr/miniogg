/* SPDX-License-Identifier: 0BSD */

/* simple demo that reads an existing Ogg Opus file,
 * demuxes the first bitstream, and remuxes using miniogg.
 * I'm assuming that all the Opus packets are 960 samples (20ms),
 * a real app would do things like, parse OpusHead and all that */

/* License text available at end of this file */

#include <ogg/ogg.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MINIOGG_IMPLEMENTATION
#include "../miniogg.h"


int main(int argc, const char* argv[] ) {
    uint8_t rawogg[4096];
    uint8_t* oggbuf = NULL;
    size_t len = 0;
    size_t plen = 0;
    size_t ppos = 0;
    size_t pused = 0;
    FILE* in = NULL;
    FILE* out = NULL;
    int r = 1;
    uint8_t serial = 0;
    uint32_t serialno = 0;
    uint64_t granulepos = 0;

    ogg_sync_state oy;
    ogg_stream_state os;
    ogg_page og;
    ogg_packet op;

    miniogg p;

    ogg_sync_init(&oy);
    miniogg_init(&p);

    if(argc < 2) {
        fprintf(stderr,"Usage: %s /path/to/source.ogg /path/to/dest.ogg\n",argv[0]);
        goto cleanup;
    }


    in = fopen(argv[1],"rb");
    if(in == NULL) {
        fprintf(stderr,"Unable to open %s for reading\n",argv[1]);
        goto cleanup;
    }

    out = fopen(argv[2],"wb");
    if(out == NULL) {
        fprintf(stderr,"Unable to open %s for writing\n",argv[2]);
        goto cleanup;
    }

    while( (len = fread(rawogg,1,4096,in))) {
        /* we'll queue up 4k chunks into the ogg_sync */
        oggbuf = (uint8_t *)ogg_sync_buffer(&oy,len);
        if(oggbuf == NULL) {
            fprintf(stderr,"error in ogg_sync_buf\n");
            goto cleanup;
        }
        memcpy(oggbuf,rawogg,len);
        if(ogg_sync_wrote(&oy,len) != 0) {
            fprintf(stderr,"error in ogg_sync_wrote\n");
            goto cleanup;
        }

        while(ogg_sync_pageout(&oy,&og)) {
            if(ogg_page_bos(&og) && !serial) {
                serialno = (uint32_t)ogg_page_serialno(&og);
                ogg_stream_init(&os,serialno);
                serial = 1;
            }

            if(serialno != (uint32_t)ogg_page_serialno(&og)) continue;

            if(ogg_stream_pagein(&os,&og) != 0) {
                fprintf(stderr,"error in ogg_stream_pagein\n");
                goto cleanup;
            }

            while(ogg_stream_packetout(&os,&op) == 1) {
                plen = op.bytes;
                ppos = 0;
                if(op.granulepos != 0) {
                    /* just assuming these are all opus packets with 960 samples
                     * per packet */
                    granulepos += 960;
                }

                while(miniogg_add_packet(&p,&op.packet[ppos],plen,granulepos,&pused)) {
                    plen -= pused;
                    ppos += pused;
                    miniogg_finish_page(&p);
                    fwrite(p.header,1,p.header_len,out);
                    fwrite(p.body,1,p.body_len,out);
                }

                if(p.granulepos == 0) { /* this is a header packet and the page should always be written */
                    miniogg_finish_page(&p);
                    fwrite(p.header,1,p.header_len,out);
                    fwrite(p.body,1,p.body_len,out);
                }
            }

            if(ogg_page_eos(&og)) goto complete;
        }
    }

    complete:

    p.eos = 1;
    miniogg_finish_page(&p);
    fwrite(p.header,1,p.header_len,out);
    fwrite(p.body,1,p.body_len,out);

    r = 0;
    cleanup:
    ogg_sync_clear(&oy);
    if(serial) ogg_stream_clear(&os);
    if(in != NULL) fclose(in);
    if(out != NULL) fclose(out);
    return r;
}

/*
BSD Zero Clause License

Copyright (c) 2023 John Regan

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

*/
