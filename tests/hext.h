#include <stdio.h>

#ifndef HEXT_H
#define HEXT_H

typedef int (*hext_read_cb)(void* data);
typedef int (*hext_write_cb)(int c, void* data);

/** Convert hext to binary using callbacks for reading and writing */
int hext_cb_to_cb(void* input, void* output, hext_read_cb read_cb, hext_write_cb write_cb);

/** Convert hext to binary using streams for input and output */
int hext_stream_to_stream(FILE* input, FILE* output);

/** Convert hext to binary using a filename for input and callback for output */
int hext_filename_to_cb(const char* filename, void* output, hext_write_cb write_cb);

/** Convert hext to binary using a filename for input and stream for output */
int hext_filename_to_stream(const char* filename, FILE* output);

/** Convert hext to binary using a filename for input and buffer for output */
int hext_filename_to_buffer(const char* filename, uint8_t *buffer, size_t buffer_len);

#endif
