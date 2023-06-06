/* SPDX-License-Identifier: 0BSD */

/* simple demo that reads an Ogg file
 * and lists the packets, including the total packet number
 * and size of the packet data. The idea is to run this alongside
 * list-packets-libogg and ensure the output is the same */

/* License text available at end of this file */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MINIOGG_IMPLEMENTATION
#include "../miniogg.h"

#define BUFFER_SIZE 1

int main(int argc, const char* argv[]) {
    size_t len;
    size_t pos;
    size_t used;
    uint8_t buffer[BUFFER_SIZE];
    miniogg ogg;

    const uint8_t *packet = NULL;
    size_t packetlen = 0;
    size_t prevlen = 0;
    uint64_t granulepos = 0;
    uint8_t cont = 0;
    uint32_t packetno = 0;
    size_t packets = 0;

    if(argc < 2) {
        fprintf(stderr,"Usage: %s /path/to/file.ogg\n",argv[0]);
        return 1;
    }

    FILE *in = fopen(argv[1],"rb");
    if(in == NULL) {
        fprintf(stderr,"Unable to open %s\n",argv[1]);
        return 1;
    }

    miniogg_init(&ogg,0);

    while( (len = fread(buffer,1,BUFFER_SIZE,in)) > 0) {
        pos = 0;

        while(miniogg_add_page(&ogg, &buffer[pos], len, &used) == 0) {

            packetno = 0;
            while( (packet = miniogg_iter_packet(&ogg,&packetlen,&granulepos,&cont)) != NULL) {
                if(cont) {
                    prevlen = packetlen;
                } else {
                    printf("packet #%lu\n",1 + packets);
                    printf("  packetlen: %lu\n",packetlen + prevlen);
                    printf("  granulepos: %lu\n",granulepos);
                    prevlen = 0;
                    packets++;
                }
                packetno++;
            }

            pos += used;
            len -= used;
        }
    }

    fclose(in);
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
