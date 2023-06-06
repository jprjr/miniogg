/* SPDX-License-Identifier: 0BSD */

/* simple demo that reads an Ogg file
 * and lists the packets, including the total packet number
 * and size of the packet data. The idea is to run this alongside
 * list-packets-miniogg and ensure the output is the same */

/* License text available at end of this file */

#include <ogg/ogg.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stdio.h>

int main(int argc, const char* argv[]) {
    ogg_sync_state oy;
    ogg_stream_state os;
    ogg_page og;
    ogg_packet op;
    size_t packetno = 0;

    char* buf;
    uint8_t *data;
    size_t len;
    FILE *f = fopen(argv[1],"rb");
    fseek(f,0,SEEK_END);
    len = ftell(f);
    fseek(f,0,SEEK_SET);
    data = malloc(len);
    fread(data,1,len,f);
    fclose(f);

    ogg_sync_init(&oy);
    buf = ogg_sync_buffer(&oy,len);
    memcpy(buf,data,len);
    ogg_sync_wrote(&oy,len);

    while(ogg_sync_pageout(&oy,&og) == 1) {
        if(ogg_page_bos(&og)) {
            ogg_stream_init(&os,ogg_page_serialno(&og));
        }
        ogg_stream_pagein(&os,&og);

        while(ogg_stream_packetout(&os,&op) == 1) {
            printf("packet #%lu\n",++packetno);
            printf("  packetlen: %lu\n",op.bytes);
            printf("  granulepos: %lu\n",op.granulepos);
        }
    }

    return 0;
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
