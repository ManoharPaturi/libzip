
/*
 compress_stream_leak.c -- exercise freeing an open compression source
 Copyright (C) 2026 Dieter Baron and Thomas Klausner

 This file is part of libzip, a library to manipulate ZIP archives.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:
 1. Redistributions of source code must retain the above copyright
 notice, this list of conditions and the following disclaimer.
 2. Redistributions in binary form must reproduce the above copyright
 notice, this list of conditions and the following disclaimer in
 the documentation and/or other materials provided with the
 distribution.
 3. The names of the authors may not be used to endorse or promote
 products derived from this software without specific prior
 written permission.

 THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS
 OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* Opens a compressed entry source and frees it without closing it
   first. The compression source is torn down while its zlib stream is
   still active; before the deallocate fix this leaked the stream state
   on every iteration, which a leak checker reports. */

#include <stdio.h>
#include <string.h>

#include "zipint.h"

/* a zip archive containing one deflated file, payload.txt */
static const unsigned char archive[] = {
    80, 75, 3, 4, 20, 0, 0, 0, 8, 0, 73, 1, 68, 93, 210, 145,
    133, 136, 15, 0, 0, 0, 112, 0, 0, 0, 11, 0, 0, 0, 112, 97,
    121, 108, 111, 97, 100, 46, 116, 120, 116, 75, 76, 74, 4, 66, 133, 68,
    44, 20, 23, 54, 65, 74, 229, 0, 80, 75, 1, 2, 20, 3, 20, 0,
    0, 0, 8, 0, 73, 1, 68, 93, 210, 145, 133, 136, 15, 0, 0, 0,
    112, 0, 0, 0, 11, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    128, 1, 0, 0, 0, 0, 112, 97, 121, 108, 111, 97, 100, 46, 116, 120,
    116, 80, 75, 5, 6, 0, 0, 0, 0, 1, 0, 1, 0, 57, 0, 0,
    0, 56, 0, 0, 0, 0, 0
};

int main(void) {
    char buffer[8192];
    zip_error_t error;
    zip_source_t *asrc;
    zip_t *za;
    int i;

    zip_error_init(&error);
    asrc = zip_source_buffer_create(archive, sizeof(archive), 0, &error);
    if (asrc == NULL) {
        fprintf(stderr, "can't create source: %s\n", zip_error_strerror(&error));
        zip_error_fini(&error);
        return 1;
    }
    za = zip_open_from_source(asrc, 0, &error);
    if (za == NULL) {
        fprintf(stderr, "can't open archive: %s\n", zip_error_strerror(&error));
        zip_source_free(asrc);
        zip_error_fini(&error);
        return 1;
    }
    zip_error_fini(&error);

    for (i = 0; i < 200; i++) {
        zip_source_t *src;

        src = zip_source_zip_file_create(za, 0, 0, 0, -1, NULL, &error);
        if (src == NULL) {
            fprintf(stderr, "can't create entry source on iteration %d: %s\n", i, zip_error_strerror(&error));
            zip_discard(za);
            return 1;
        }
        if (zip_source_open(src) < 0) {
            fprintf(stderr, "can't open entry source on iteration %d\n", i);
            zip_source_free(src);
            zip_discard(za);
            return 1;
        }
        while (zip_source_read(src, buffer, sizeof(buffer)) > 0) {
            ;
        }
        /* deliberately no zip_source_close: this frees the source while it is open */
        zip_source_free(src);
    }

    zip_discard(za);
    return 0;
}
