/*

  hext: a markup language and tool for describing binary files

  Copyright 2016 Nicholas Humfrey

*/

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

#include "hext.h"



static int ascii_to_hex(char c)
{
    c |= 0x20;

    if (c >= '0' && c <= '9') {
        return c - '0';
    } else if (c >= 'a' && c <= 'f') {
        return (c - 'a') + 10;
    } else {
        return -1;
    }
}

/* Based on: https://en.wikipedia.org/wiki/Escape_sequences_in_C */
static int escape_to_hex(int c)
{
    switch(c) {
        case '0':  return 0x00;
        case 'a':  return 0x07;
        case 'b':  return 0x08;
        case 'f':  return 0x0C;
        case 'n':  return 0x0A;
        case 'r':  return 0x0D;
        case 't':  return 0x09;
        case 'v':  return 0x0B;
        case '\\': return 0x5C;
        case '\'': return 0x27;
        case '"':  return 0x22;
        case '?':  return 0x3F;
        default:
            return -1;
    }
}

int hext_cb_to_cb(void* input, void* output, hext_read_cb read_cb, hext_write_cb write_cb)
{
    size_t count = 0;
    int readahead = 0;

    while (1) {
        int chr;

        if (readahead) {
            chr = readahead;
            readahead = 0;
        } else {
            chr = read_cb(input);
        }

        if (chr == EOF) {
            break;

        } else if (isspace(chr) || chr == ':' || chr == '-') {
            /* Ignore */
            continue;

        } else if (chr == '#') {
            /* Ignore the rest of the line */
            while (1) {
                int chr2 = read_cb(input);
                if (chr2 == EOF || chr2 == '\n' || chr2 == '\r')
                    break;
            }

        } else if (isxdigit(chr)) {
            int chr2 = read_cb(input);
            if (!isxdigit(chr2)) {
                fprintf(stderr, "Error: got non-hex digit after hex digit: '%c'\n", chr2);
                break;
            }

            write_cb((ascii_to_hex(chr) << 4) + ascii_to_hex(chr2), output);
            count++;

        } else if (chr == '.') {
            /* 8-bit Integer */
            char chars[5];
            int digits = 0;

            while (digits < 4) {
                int chr2 = read_cb(input);
                if (isdigit(chr2) || chr2 == '-') {
                    chars[digits++] = chr2;
                } else {
                    /* Store the extra character for later */
                    readahead = chr2;
                    break;
                }
            }

            chars[digits] = '\0';

            if (digits < 1 || digits > 4) {
                fprintf(stderr, "Error: invalid integer: %s\n", chars);
                break;
            } else {
                int num = atoi(chars);
                write_cb((uint8_t)num & 0xFF, output);
                count++;
            }

        } else if (chr == '"') {
            while (1) {
                int chr2 = read_cb(input);
                if (chr2 == EOF || chr2 == '"') {
                    break;
                } else if (chr2 == '\\') {
                    int chr3 = read_cb(input);
                    int escaped = escape_to_hex(chr3);
                    if (escaped < 0) {
                        fprintf(stderr, "Error: invalid escape sequence '%c'\n", chr3);
                        break;
                    } else {
                        write_cb(escaped, output);
                        count++;
                    }
                } else {
                    write_cb(chr2, output);
                    count++;
                }
            }

        } else if (chr == '\\') {
            int chr2 = read_cb(input);
            if (chr2 == EOF) {
                break;
            } else {
                int escaped = escape_to_hex(chr2);
                if (escaped < 0) {
                    fprintf(stderr, "Error: invalid escape sequence '%c'\n", chr2);
                    break;
                } else {
                    write_cb(escaped, output);
                    count++;
                }
            }

        } else {
            fprintf(stderr, "Error: unrecognised character in input: '%c'\n", chr);
            break;
        }
    }

    return count;
}

int hext_stream_to_stream(FILE* input, FILE* output)
{
    /* Explicit cast to avoid compiler warning */
    hext_read_cb read_cb = (hext_read_cb)fgetc;
    hext_write_cb write_cb = (hext_write_cb)fputc;

    return hext_cb_to_cb(input, output, read_cb, write_cb);
}

int hext_filename_to_cb(const char* filename, void* output, hext_write_cb write_cb)
{
    /* Explicit cast to avoid compiler warning */
    hext_read_cb read_cb = (hext_read_cb)fgetc;
    FILE *input = NULL;
    int len = 0;

    input = fopen(filename, "rb");
    if (!input) {
        perror("Failed to open input file");
        return -1;
    }

    len = hext_cb_to_cb(input, output, read_cb, write_cb);

    fclose(input);
    return len;
}

int hext_filename_to_stream(const char* filename, FILE* output)
{
    hext_write_cb write_cb = (hext_write_cb)fputc;

    return hext_filename_to_cb(filename, output, write_cb);
}

struct buffer_write_struct {
    uint8_t *buffer;
    size_t buffer_used;
    size_t buffer_len;
};

static int write_buffer_cb(int c, void* data)
{
    struct buffer_write_struct *bws = (struct buffer_write_struct*)data;

    if (bws->buffer_used + 1 < bws->buffer_len) {
        bws->buffer[bws->buffer_used] = c;
        bws->buffer_used++;
        return c;
    } else {
        /* Buffer isn't big enough */
        return -1;
    }
}

int hext_filename_to_buffer(const char* filename, uint8_t *buffer, size_t buffer_len)
{
    struct buffer_write_struct bws = {NULL, 0, 0};
    int len = 0;

    bws.buffer = buffer;
    bws.buffer_len = buffer_len;

    len = hext_filename_to_cb(filename, &bws, write_buffer_cb);

    return len;
}

