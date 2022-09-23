/*
 * main.c
 *
 * Copyright (C) 2018 bzt (bztsrc@gitlab)
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * @brief: example, tutorial on how to use easyjpeg.h
 */
#include <stdio.h>

#define EASYJPEG_IMPLEMENTATION
/*#define EASYJPEG_NOLOAD*/
/*#define EASYJPEG_NOSAVE*/
/*#define EASYJPEG_NORESIZE*/
/*#define EASYJPEG_DOWNSCALE_ONLY*/
#include "easyjpeg.h"

/**
 * Main function
 */
int main(int argc, char **argv)
{
    image_t *in, *out;
    FILE *f;
    /* compressed JPEG buffer */
    char *buf=NULL;
    int len;

    /* check arguments */
    if (argc<2 || argv[1]==NULL) {
        printf("%s <test jpeg file>\n", argv[0]);
        return 0;
    }

    f = fopen(argv[1], "rb");
    if (f) {
        /* load the jpeg */
        in = image_read_file(f);
        fclose(f);
        if (!in) {
            fprintf(stderr, "unable to read jpeg\n%s\n", easyjpeg_errorstr);
            return 1;
        }
        printf("image_read_file() successful, dimensions %d x %d\n", in->w, in->h);

        /* write out with same dimensions but fixed quality */
        f = fopen("test_same.jpg","wb");
        if (!f || !image_write_file(in, 75, f)) {
            fprintf(stderr, "unable to write test_same.jpg\n%s\n", easyjpeg_errorstr);
            return 1;
        }
        fclose(f);
        printf("image_write_file() successful\n");
        image_free(in);
        in = NULL;

        /* read the file again, this time decompress from memory */
        f = fopen(argv[1], "rb");
        if(f) {
            fseek(f, 0, SEEK_END);
            len = (int)ftell(f);
            fseek(f, 0, SEEK_SET);
            buf = malloc(len);
            if(buf) {
                fread(buf, len, 1, f);
                /* decompress */
                in = image_read_mem(buf, len);
                free(buf);
                buf = NULL;
            }
            fclose(f);
        }
        if (!in) {
            fprintf(stderr, "unable to read jpeg\n%s\n", easyjpeg_errorstr);
            return 1;
        }
        printf("image_read_mem() successful\n");

        /* create a big image */
        out = image_resize(in, 2048, 2048, EASYJPEG_RESIZE);
        if (!out) {
            fprintf(stderr, "unable to resize\n");
            return 2;
        }
        printf("image_resize() successful, new dimensions %d x %d\n", out->w, out->h);
        f = fopen("test_big.jpg","wb");
        if (!f || !image_write_file(out,75,f)) {
            fprintf(stderr, "unable to write test_big.jpg\n%s\n", easyjpeg_errorstr);
            return 1;
        }
        fclose(f);
        printf("image_write_file() successful\n");
        image_free(out);

        /* create a thumbnail, this time into memory */
        out = image_resize(in, 128, 128, EASYJPEG_CROP);
        if (!out) {
            fprintf(stderr, "unable to resize\n");
            return 2;
        }
        printf("image_resize() successful, new dimensions %d x %d\n", out->w, out->h);
        len=image_write_mem(out, 60, &buf);
        if(len>0 && buf) {
            printf("image_write_mem() successful, compressed jpeg size: %d\n",len);
            f = fopen("test_thumb.jpg","wb");
            if (!f || !fwrite(buf, len, 1, f)) {
                fprintf(stderr, "unable to write test_thumb.jpg\n");
                return 1;
            }
            fclose(f);
        } else {
            fprintf(stderr, "unable to write to memory\n%s\n", easyjpeg_errorstr);
            return 2;
        }

    } else {
        fprintf(stderr, "unable to open %s\n", argv[1]);
        return 1;
    }
    return 0;
}
