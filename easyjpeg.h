/*
 * easyjpeg.h
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
 * @brief: wrapper around libjpeg interface
 */
#ifndef _EASYJPEG_IMAGE_H_
#define _EASYJPEG_IMAGE_H_

#ifdef  __cplusplus
extern "C" {
#endif

/*****************************************************************************
 *                        In memory image structure                          *
 *****************************************************************************/
typedef struct {
    int w;                  /* width */
    int h;                  /* height */
    unsigned int *d;        /* each pixel stored as 4 bytes */
} image_t;

/*****************************************************************************
 *                           Public EasyJpeg API                             *
 *****************************************************************************/
/* string to store the error messages */
extern char easyjpeg_errorstr[];

/* read from JPEG into image_t */
image_t *image_read_file(FILE *f);
image_t *image_read_mem(char *buf, int size);

/* write from image_t into JPEG */
int image_write_file(image_t *in, int quality, FILE *f);
int image_write_mem(image_t *in, int quality, char **buf);

/* misc functions */
void image_free(image_t *img);
#define EASYJPEG_RESIZE 0
#define EASYJPEG_CROP   1
image_t *image_resize(image_t *in, int w, int h, int crop);

/*****************************************************************************
 *                 Boring implementation details, don't look                 *
 *****************************************************************************/
#ifdef EASYJPEG_IMPLEMENTATION
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <jpeglib.h>

/* needed by jpeglib */
char easyjpeg_errorstr[JMSG_LENGTH_MAX];
jmp_buf easyjpeg_jmpbuf;
static void easyjpeg_silent(j_common_ptr cinfo) {
    (*cinfo->err->format_message) (cinfo, easyjpeg_errorstr);
    jpeg_destroy(cinfo);
    longjmp(easyjpeg_jmpbuf, 1);
}

/**
 * Free an image from memory
 */
void image_free(image_t *img)
{
    if(img) {
        if(img->d) free(img->d);
        free(img);
    }
}

#ifndef EASYJPEG_NOLOAD
/**
 * Load a jpeg image
 */
#define image_read_file(f) image_read((void*)f, 1, 0)
#define image_read_mem(buf, size) image_read(buf, 0, size)
image_t *image_read(void *f, int src, int size)
{
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    volatile JSAMPROW row=0;
    image_t *out=NULL;
    JSAMPROW rowptr[1];
    JDIMENSION i,j;
    unsigned int *pix;
    if(f==NULL) return NULL;
    out=malloc(sizeof(image_t));
    if(out==NULL) return NULL;
    memset(out,0,sizeof(image_t));
    memset(&cinfo,0,sizeof(cinfo));
    memset(&jerr,0,sizeof(jerr));
    memset(&easyjpeg_errorstr,0,sizeof(easyjpeg_errorstr));
    cinfo.err=jpeg_std_error(&jerr);
    cinfo.err->output_message = easyjpeg_silent;
    cinfo.err->error_exit = easyjpeg_silent;
    if(setjmp(easyjpeg_jmpbuf) != 0) {
        if(row) free(row);
        image_free(out);
        return 0;
    }
    jpeg_create_decompress(&cinfo);
    if(src) {
        jpeg_stdio_src(&cinfo,(FILE *)f);
    } else {
        jpeg_mem_src(&cinfo,(unsigned char*)f,size);
    }

    if(jpeg_read_header(&cinfo,1)!=JPEG_HEADER_OK) {
        image_free(out);
        return NULL;
    }
    cinfo.out_color_space=JCS_RGB;
    jpeg_start_decompress(&cinfo);
    out->w=cinfo.output_width;
    out->h=cinfo.output_height;
    out->d=malloc(out->w*out->h*sizeof(unsigned int));
    row=calloc(cinfo.output_width*cinfo.output_components, sizeof(JSAMPLE));
    if(out->d==NULL||row==NULL) {
err:    jpeg_destroy_decompress(&cinfo);
        if(row) free(row);
        image_free(out);
        return NULL;
    }
    rowptr[0]=row;
    pix=out->d;
    for(i=0;i<cinfo.output_height;i++) {
        register JSAMPROW currow=row;
        if(jpeg_read_scanlines(&cinfo, rowptr, 1)!=1) goto err;
        for(j=0;j<cinfo.output_width;j++,currow+=cinfo.output_components,pix++) {
            *pix=0;
            if(cinfo.jpeg_color_space==JCS_GRAYSCALE) {
                ((unsigned char*)pix)[0]=*currow;
                ((unsigned char*)pix)[1]=*currow;
                ((unsigned char*)pix)[2]=*currow;
            } else {
                memcpy(pix,currow,cinfo.output_components);
            }
        }
    }
    free(row);
    jpeg_destroy_decompress(&cinfo);
    return out;
}
#endif

#ifndef EASYJPEG_NOSAVE
/**
 * Write out an image in jpeg format
 */
#define image_write_file(in, quality, f) image_write(in, quality,(void**)&f, 1)
#define image_write_mem(in, quality, buf) image_write(in, quality,(void**)buf, 0)
int image_write(image_t *in, int quality, void **f, int dst)
{
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    volatile JSAMPROW row = 0;
    JSAMPROW rowptr[1];
    JDIMENSION i,j;
    unsigned int *pix;
    unsigned long int ret;
    if(in==NULL || in->d==NULL || in->w<=0 || in->h<=0) return 0;
    if(quality<0) quality=0;
    if(quality>100) quality=100;
    memset(&cinfo,0,sizeof(cinfo));
    memset(&jerr,0,sizeof(jerr));
    memset(&easyjpeg_errorstr,0,sizeof(easyjpeg_errorstr));
    cinfo.err=jpeg_std_error(&jerr);
    cinfo.err->output_message = easyjpeg_silent;
    cinfo.err->error_exit = easyjpeg_silent;
    if(setjmp(easyjpeg_jmpbuf) != 0) {
        if(row) free(row);
        if(*f) free(*f);
        *f=NULL;
        return 0;
    }
    jpeg_create_compress(&cinfo);
    if(dst) {
        if(!*f) goto err;
        ret=1;
        jpeg_stdio_dest(&cinfo, (FILE*)(*f));
    } else {
        ret=0;
        jpeg_mem_dest(&cinfo, (unsigned char **)f, &ret);
    }
    cinfo.image_width=in->w;
    cinfo.image_height=in->h;
    cinfo.input_components=3;
    cinfo.in_color_space=JCS_RGB;
    jpeg_set_defaults(&cinfo);
    if(quality>0) {
        jpeg_set_quality(&cinfo,quality,1);
        if(quality>=90) {
            cinfo.comp_info[0].h_samp_factor=1;
            cinfo.comp_info[0].v_samp_factor=1;
        }
    }
    row=calloc(cinfo.image_width*cinfo.input_components, sizeof(JSAMPLE));
    if(row==NULL) {
err:    jpeg_destroy_compress(&cinfo);
        if(row) free(row);
        return 0;
    }
    rowptr[0]=row;
    pix=in->d;
    jpeg_start_compress(&cinfo,1);
    for(i=0;i<in->h;i++) {
        register JSAMPROW currow=row;
        for(j=0;j<in->w;j++,currow+=cinfo.input_components,pix++) {
            memcpy(currow,pix,cinfo.input_components);
        }
        if(jpeg_write_scanlines(&cinfo, rowptr, 1)!=1) goto err;
    }
    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
    return ret;
}
#endif

#ifndef EASYJPEG_NORESIZE
/**
 * Resize an image with keeping it's aspect ratio and optionally cropping it
 */
image_t *image_resize(image_t *in, int w, int h, int crop)
{
    int sx=0,sy=0,sw,sh,i,j,rs;
    unsigned int *pix;
#ifdef EASYJPEG_DOWNSCALE_ONLY
    int dx,dy;
#else
    double dx,dy;
#endif
    image_t *out=NULL;
    if(in==NULL || in->d==NULL || in->w<=0 || in->h<=0 || w<1 || h<1) return 0;
    out=malloc(sizeof(image_t));
    if(out==NULL) return NULL;
    memset(out,0,sizeof(image_t));
    if(crop) {
        out->w=w;
        out->h=h;
        if(in->w<=in->h) {
            sw=in->w;
            sh=h*in->w/w;
            sx=0;
            sy=(in->h-sh)/2;
        } else {
            sw=w*in->h/h;
            sh=in->h;
            sx=(in->w-sw)/2;
            sy=0;
        }
    } else {
        out->w=in->w<in->h?in->w*h/in->h:w;
        out->h=in->w<in->h?h:w*in->h/in->w;
        sx=sy=0;
        sw=in->w;
        sh=in->h;
    }
    out->d=malloc((out->w*out->h+1)*sizeof(unsigned int));
    if(!out->d) {
        image_free(out);
        return NULL;
    }
#ifdef EASYJPEG_DOWNSCALE_ONLY
    dx=sw/out->w;
    dy=sh/out->h;
#else
    dx=(double)sw/(double)out->w;
    dy=(double)sh/(double)out->h;
#endif
    pix=out->d;
/*printf("(%d,%d)(%d,%d,%d,%d) -> (%d,%d) (%g,%g)\n",in->w,in->h,sx,sy,sw,sh,out->w,out->h,dx,dy);*/
    for(i=0;i<out->h;i++) {
        rs=(((int)(i*dy)+sy)*in->w)+sx;
        for(j=0;j<out->w;j++) {
            *pix++ = in->d[(int)(j*dx)+rs];
        }
    }
    return out;
}
#endif

#endif

#ifdef  __cplusplus
}
#endif

#endif
