EasyJpeg
========

This C/C++ header file is a wrapper around IJG's [jpeglib](http://www.ijg.org), which has a terrible, nightmareish API,
even involving the horrors of setjmp.

This was written in ANSI C (C89), and provides a nice and easy way for loading and saving JPEG files, with one struct
definition and six functions only.

Intended audience are embedded software or game developers in need of JPEG support, but where a several megabytes
third party library (like libGD or ImageMagick) is not feasable.

How to Use
----------

In each of your source files, where you want to access the library, include the header:

```c
#include <easyjpeg.h>
```

In *exactly one* source file, where you want the code to be placed, specify a define too:

```c
#define EASYJPEG_IMPLEMENTATION
#include <easyjpeg.h>
```

And then link your program with

```sh
-ljpeg
```

For examples on the API usage, see main.c which tests the library's functions. Detailed description below.

Library Functions
-----------------

The library stores an image in memory in the following, quite self-explanatory structure:

```c
typedef struct {
    int w;                  /* width */
    int h;                  /* height */
    unsigned int *d;        /* data, each pixel stored as 4 bytes */
} image_t;
```

EasyJpeg gives you the following API to manipulate these structs:

### Loading images

```c
image_t *image_read_file(FILE *f);
image_t *image_read_mem(char *buf, int size);
```

Convert a JPEG compressed picture into a newly allocated image structure. Returns NULL on error, and exact error message
is in `easyjpeg_errorstr`.  Grayscale images will be converted to sRGB automatically on load, other obscure pixel formats
(CMYK, YCCK etc.) untouched.

Can be turned off with the define `EASYJPEG_NOLOAD`.

### Saving images

```c
int image_write_file(image_t *in, int quality, FILE *f);
int image_write_mem(image_t *in, int quality, char **buf);
```

Compress an in memory image into a JPEG file with the given quality (1-100, 0=default). Returns zero on error. The output
buffer will be allocated as needed, and buffer size is returned. The file variant returns one on success. These functions
intentinally DO NOT save any meta information (like comments, EXIF, photo shoot time or GPS longitude, latidute etc.) to
protect privacy. For simplicity, these only support sRGB pixel format (most widely used on the web). If you have loaded
a JPEG with different format (like CMYK), then you have to convert every pixel in image_t->d buffer before you can save
it color-correctly.

Can be turned off with the define `EASYJPEG_NOSAVE`.

### Resizing images

```c
image_t *image_resize(image_t *in, int w, int h, int crop);
```

If required, you can use this function to quickly resize an in memory image. It's not optimal, uses the simpliest method,
the closest neightbour algorithm without resampling. But could be handy to generate thumbnails as it has absolutely no
dependencies, and it's pixel format agnostic. The last crop parameter can be either EASYJPEG_RESIZE, when the resulting
image will be no bigger than `w` x `h`, keeping the original picture's aspect ratio. Or it can be EASYJPEG_CROP, then the
resulting image will be exactly `w` x `h` pixels in dimension, and the picture will be cropped from the middle of the
scaled image (portait images will be first scaled to match width, landscapes to match height before being cropped). If
you define `EASYJPEG_DOWNSCALE_ONLY` before including EasyJpeg header, then only integer arithmetic will be used (jpeglib
itself can be compiled without any floating-number support, which could be important on embedded systems).

````
EASYJPEG_RESIZE
+-+
| |   @@@@     #          +-------+   @@@@    ####
| | + @@@@ ->  #          +-------+ + @@@@ ->
| |   @@@@     #                      @@@@
+-+

EASYJPEG_CROP
+-+           ....
| |   @@@@    ####        +-------+   @@@@    .....####.....
| | + @@@@ -> ####        +-------+ + @@@@ -> .....####.....
| |   @@@@    ####                    @@@@    .....####.....
+-+           ....

+- original image, @ desired dimension, # resulting image, . cropped parts
````

Can be turned off with the define `EASYJPEG_NORESIZE`.

### Destroying images

```c
void image_free(image_t *img);
```

And finally, a function to free an allocated image structure.

License
-------

MIT licensed. Use as you please.

Author
------

bzt (bztsrc@gitlab)
