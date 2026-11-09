// Copyright 2015 Intel Corporation
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice (including the next
// paragraph) shall be included in all copies or substantial portions of the
// Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

/// \file
/// \brief Test that concurrent test output is not corrupted.
///
/// Each subtest repeatedly writes very large strings to the test logs. The
/// test itself, though, cannot verify that its output is not corrupted. That
/// requires external inspection.

#include <sched.h>
#include <stdio.h>

#include "tapi/t.h"

/// Make a big string filled with a repeated character.
static char *
mk_big_str(char c)
{
    char *s = xmalloc(8096);
    t_cleanup_push_free(s);
    memset(s, c, 8096);
    s[8096 - 1] = 0;
    return s;
}

static void
test_fragmented_output(char fill)
{
    char payload[8192];
    FILE *streams[] = { stdout, stderr };

    memset(payload, fill, sizeof(payload));
    for (unsigned stream = 0; stream < ARRAY_LENGTH(streams); ++stream) {
        FILE *file = streams[stream];
        for (int iteration = 0; iteration < 16; ++iteration) {
            fprintf(file, "split-%c:", fill);
            fwrite(payload, 1, sizeof(payload) / 2, file);
            fflush(file);
            // Give the dispatcher a chance to read a line before it is complete.
            sched_yield();
            fwrite(payload, 1, sizeof(payload) / 2, file);
            fputc('\n', file);
            fflush(file);
        }
    }

    // Exceed the buffering limit without a newline to exercise chunked output.
    memset(payload, fill == 'a' ? 'x' : 'y', sizeof(payload));
    for (int iteration = 0; iteration < 16; ++iteration)
        fwrite(payload, 1, sizeof(payload), stderr);
    fputc('\n', stderr);
    // This final partial line must be forwarded when the worker closes stderr.
    fprintf(stderr, "tail-%c", fill);
    fflush(stderr);
}

static void
test_logi_a(void)
{
    char *a = mk_big_str('a');

    for (int i = 0; i < 1024; ++i) {
        logi("%s", a);
    }
    test_fragmented_output('a');
}

test_define {
    .name = "self.concurrent-output.logi-a",
    .start = test_logi_a,
    .no_image = true,
};

static void
test_logi_b(void)
{
    char *b = mk_big_str('b');

    for (int i = 0; i < 1024; ++i) {
        logi("%s", b);
    }
    test_fragmented_output('b');
}

test_define {
    .name = "self.concurrent-output.logi-b",
    .start = test_logi_b,
    .no_image = true,
};
