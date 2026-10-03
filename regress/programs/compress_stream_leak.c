/*
 compress_stream_leak.c -- exercise free without close on compression sources
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

/* Creates an archive with a deflated entry, then repeatedly opens the
   entry and discards the archive without closing the file first. The
   compression source is freed while its zlib stream is still active;
   before the deallocate fix this leaked the stream state on every
   iteration, which a leak checker reports. */

#include <stdio.h>
#include <string.h>

#include "zip.h"

static const char payload[] = "ababab ababab ababab ababab\n";

int main(void) {
    const char archive_name[] = "compress_stream_leak.zip";
    char buffer[8192];
    zip_t *za;
    zip_source_t *src;
    int i;

    za = zip_open(archive_name, ZIP_CREATE | ZIP_TRUNCATE, NULL);
    if (za == NULL) {
        fprintf(stderr, "can't create archive\n");
        return 1;
    }
    src = zip_source_buffer(za, payload, sizeof(payload) - 1, 0);
    if (src == NULL || zip_file_add(za, "payload.txt", src, ZIP_FL_ENC_UTF_8) < 0 || zip_set_file_compression(za, 0, ZIP_CM_DEFLATE, 1) < 0 || zip_close(za) < 0) {
        fprintf(stderr, "can't populate archive\n");
        zip_discard(za);
        return 1;
    }

    for (i = 0; i < 200; i++) {
        za = zip_open(archive_name, 0, NULL);
        if (za == NULL) {
            fprintf(stderr, "can't open archive on iteration %d\n", i);
            return 1;
        }
        zip_file_t *zf = zip_fopen(za, "payload.txt", 0);
        if (zf == NULL) {
            fprintf(stderr, "can't open file on iteration %d\n", i);
            zip_discard(za);
            return 1;
        }
        (void)zip_fread(zf, buffer, sizeof(buffer));
        /* deliberately no zip_fclose: this exercises freeing the source while it is open */
        zip_discard(za);
    }

    remove(archive_name);
    return 0;
}
