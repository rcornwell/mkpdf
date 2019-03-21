//
//
// Copyright 2019 Richard P. Cornwell All Rights Reserved,
//
// The software is provided "as is", without warranty of any kind, express
// or implied, including but not limited to the warranties of
// merchantability, fitness for a particular purpose and non-infringement.
// In no event shall Richard Cornwell be liable for any claim, damages
// or other liability, whether in an action of contract, tort or otherwise,
// arising from, out of or in connection with the software or the use or other
// dealings in the software.
//
// Permission to use, copy, and distribute this software and its
// documentation for non commercial use is hereby granted,
// provided that the above copyright notice appear in all copies and that
// both that copyright notice and this permission notice appear in
// supporting documentation.
//
// The sale, resale, or use of this program for profit without the
// express written consent of the author Richard Cornwell is forbidden.
//
// This program uses a XML control file to generate a PDF file. This is used
// to convert listings and images into a more easy to read format. This program
// is also capable of doing limited black and white processing to scanned images
// to make them easier to read.

// Process images

#include <stdio.h>
#include <string.h>
#include <png.h>
#include <sys/stat.h>
#include <unistd.h>
#include <math.h>

#include "Obj.h"
#include "Stream.h"
#include "Image.h"

extern int      verbose;

//
// Load an image file into memory. Currently supports PNG files.
int
Image::open(xmlChar *name)
{
    FILE            *f;
    png_structp     png_ptr;
    png_infop       info_ptr, end_ptr;
    png_bytep       *row_pointers;
    int             i;
    unsigned char   *dp;

    f = fopen((const char *)name, "r");
    if (!f) {
        fprintf(stderr, "Could not open image %s\n", name);
        return 0;
    }
    if (verbose)
        fprintf(stderr, "Reading image %s\n", name);
    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
            NULL, NULL, NULL);
    if (!png_ptr) {
        fclose(f);
        fprintf(stderr, "Could not create new PNG object for %s\n", name);
        return 0;
    }

    info_ptr = png_create_info_struct(png_ptr);
    end_ptr = png_create_info_struct(png_ptr);
    if(info_ptr == 0 || end_ptr == 0) {
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_ptr);
        fclose(f);
        fprintf(stderr, "Could not create new PNG struct for %s\n", name);
        return 0;
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_ptr);
        fclose(f);
        fprintf(stderr, "Error reading PNG file %s\n", name);
        return 0;
    }

    png_init_io(png_ptr, f);
    png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY |
            PNG_TRANSFORM_STRIP_16|PNG_TRANSFORM_STRIP_ALPHA, NULL);
    width = png_get_image_width(png_ptr, info_ptr);
    height = png_get_image_height(png_ptr, info_ptr);
    row_width = png_get_rowbytes(png_ptr, info_ptr);
    bpp = png_get_bit_depth(png_ptr, info_ptr);
    row_pointers = png_get_rows(png_ptr, info_ptr);
    data = new unsigned char[row_width * height];
    dp = data;
    for(i = 0; i < height; i++) {
        memcpy(dp, row_pointers[i], row_width);
        dp += row_width;
    }
    png_destroy_read_struct(&png_ptr, &info_ptr, &end_ptr);
    fclose(f);
    if (verbose)
        fprintf(stderr, "   Width %d (%d) Height %d BPP %d\n", width,
                    row_width, height, bpp);

    // If gray scale image, create histogram and map for image.
    if (bpp == 8) {
       int min = 128;
       int max = 128;
       unsigned char map[256];
       float  scale;
       memset(hist, 0, sizeof(hist));
       dp = data;
       for (i = row_width * height; i > 0; i--) 
            hist[(int)*dp++]++;
       for (i = 0; i < 128; i++) {
            if (hist[i] != 0) {
               min = i;
               break;
            }
       }
       for (i = 255; i > 128; i--) {
            if (hist[i] != 0) {
               max = i;
               break;
            }
       }
       scale = 256.0 / ((float)(max - min));
       for (i = 0; i <= 255; i++) {
            int k = (int)(((float)(i - min))*scale);
            if (k < 0)
                k = 0;
            if (k > 255)
                k = 255;
            map[i] = k;
       }

       memset(hist, 0, sizeof(hist));
       dp = data;
       for (i = (row_width * height)-1; i > 0; i--, dp++) 
            hist[(int)(*dp = map[(int)*dp])]++;
    }
    return 1;
}

//
// Compute average brightness, then set threshold for B/W conversion to this value.
void
Image::avg(int value)
{
    int     i, j, k, th;

    if (bpp != 8) 
        return;

    th = 127;
    for(k = 64; k > 0; k /= 2) {
        int         sl, sh;
        int         nth;
        sl = sh = 0;
        for(i = 1; i < 255; i++) {
            if (i < th)
               sl += hist[i];
            else 
               sh += hist[i];
        }
        nth = 8 * sl;
        nth -= sh;
        if (nth != 0) {
            if (nth > 0) 
                j = th - k;
            else
                j = th + k;
            if (j == th)
                break;
            th = j;
        }
    };

    if (verbose)
        fprintf(stderr, " Thresh = %d %d -> %d\n", th, value, th+value);
    thresh(th + value);
}

//
// All pixels below the threshhold are made black, all above white.
// Image type made into B/W image.
void
Image::thresh(int value)
{
    unsigned char   *nimage;
    unsigned char   *dp, *op;
    int             rw;
    int             i, j, k;
    
    if (bpp != 8) 
       return;

    rw = (width >> 3) + ((width & 7) != 0);
    nimage = new unsigned char[rw * height];
    memset(nimage, 0, rw * height);
    dp = data;
    for(j = 0; j < height; j++) {
        op = &nimage[j * rw];
        k = 7;
        for (i = 0; i < row_width; i++) {
             if (*dp++ > value) 
                 *op |= 1 << k;
             if (k-- == 0) {
                 op++;
                 k = 7;
             }
        }
    }
    delete[] data;
    data = nimage;
    row_width = rw;
    bpp = 1;
}

//
// Save an image on the given Stream.
Obj
*Image::save(Stream *strm)
{
    strm->appendData((char *)data, row_width * height);
    strm->open("XObject/Subtype/Image");
    strm->put("Width", width);
    strm->put("Height", height);
    strm->put("\n/ColorSpace/DeviceGray");
    strm->put("BitsPerComponent", bpp);
    strm->close();
    return strm->obj;
}

// Explode row into array of pixels.
void
Image::unpackrow(int width, unsigned char *in, unsigned char *out)
{
   int             i, k;
   int             mask;
   unsigned char   cur;

   if (bpp == 8) {
      memcpy(out, in, width);
      return;
   }

   k = 8;
   mask = (1 << bpp) - 1;
   cur = *in++;
   for (i = 0; i < width; i++) {
        k -= bpp;
        *out++ = (cur >> k) & mask;
        if (k == 0) {
            k = 8;
            cur = *in++;
       }
   }
}


// Pack exploded row back into imae.
void
Image::packrow(int width, unsigned char *in, unsigned char *out)
{
    int             i, k;
    int             mask;
    unsigned char   cur;

    if (bpp == 8) {
        memcpy(out, in, width);
        return;
    }

    k = 7;
    mask = (1 << bpp) - 1;
    cur = 0;
    for (i = 0; i < width; i++) {
         cur <<= bpp;
         cur |= *in++ & mask;
         k -= bpp;
         if (k < 0) {
             *out++=cur;
             k = 7;
             cur = 0;
        }
    }
    if (k != 7) {
        while(k > 0) {
            cur <<= bpp;
            k -= bpp;
        }
        *out++ = cur;
    }
}

//
// Basic image rotation, right 90 degrees.
void
Image::rotater90()
{
    transpose();
    reverse();
}

//
// Basic image rotation, left 90 degrees.
void
Image::rotatel90()
{
   transpose();
   flip();
}

// Basic image rotation, 180 degrees.
void
Image::rotate180()
{
   flip();
   reverse();
}

// Flip an image by 90 degrees.
// Exhange x and y axis.
void
Image::transpose()
{
    int             i, j;
    unsigned char   row_buffer[width];
    unsigned char   *dp;
    unsigned char   *temp;
    unsigned char   *tp;

    dp = data;
    temp = new unsigned char[width * height];
    for(i = 0; i < height; i++) {
        unpackrow(width, dp, row_buffer);
        tp = &temp[i];
        for (j = 0; j < width; j++) {
            *tp = row_buffer[j];
             tp += height;
        }
        dp += row_width;
    }
    i = height;
    height = width;
    width = i;
    row_width = (width * bpp) / 8;
    if ((row_width * bpp) < width)
        row_width++;
    dp = data;
    tp = temp;
    for(i = 0; i < height; i++) {
        packrow(width, tp, dp);
        dp += row_width;
        tp += width;
    }
    delete[] temp;
}

// Row reverse and image.
void
Image::reverse()
{
    int             i, j, k;
    unsigned char   row_buffer[width];
    unsigned char   *dp;

    dp = data;
    for(i = 0; i < height; i++) {
        unpackrow(width, dp, row_buffer);
        for (j = 0, k = width-1; j < k; j++, k--) {
            unsigned char    t;
            t = row_buffer[j];
            row_buffer[j] = row_buffer[k];
            row_buffer[k] = t;
        }
        packrow(width, row_buffer, dp);
        dp += row_width;
    }
}

// flip image up side down.
void
Image::flip()
{
    int             i, j;
    unsigned char   row_buffer[row_width];

    for(i = 0, j = height-1; i < j; i++, j--) {
        memcpy(row_buffer, &data[i * row_width], row_width);
        memcpy(&data[i * row_width], &data[j * row_width], row_width);
        memcpy(&data[j * row_width], row_buffer, row_width);
    }
}

// Perform an unsharpen function on the image.
void
Image::unsharp(int amount, int radius, int thresh)
{
    unsigned char   *temp, *temp2;
    unsigned char   *dp;
    unsigned char   *tp;
    int             i, j, k;
    int             acc;
    int             min = 128;
    int             max = 128;
    unsigned char   map[256];
    float           scale;

    if (verbose)
        fprintf(stderr, "    unsharp %d %d %d\n", amount, radius, thresh);
    temp = new unsigned char [width * height];
    temp2 = new unsigned char [width * height];
    dp = data;
    tp = temp;
    for(i = 0; i < height; i++) {
        unpackrow(width, dp, tp);
        dp += row_width;
        tp += width;
    }
    k = 0;
    while(--radius > 0) {
        if (k) {
            tp = temp2;
            dp = temp;
        } else {
            tp = temp;
            dp = temp2;
        } 
        k = !k;
        acc = 4*((int)tp[0]) + 2*((int)tp[1]);
        acc += 2*((int)tp[width]) + ((int)tp[width+1]);
        acc += 255 * 5;
        acc /= 16;
        *dp++ = (unsigned char)acc;
        tp++;
        for(j = 1; j < width-1; j++) {
            acc = 2*((int)tp[-1]);
            acc += 4*((int)tp[0]) + 2*((int)tp[1]) + ((int)tp[width-1]);
            acc += 2*((int)tp[width]) + ((int)tp[width+1]);
            acc += 255 * 3;
            acc /= 16;
            *dp++ = (unsigned char)acc;
            tp++;
        }
        acc = 2*((int)tp[-1])  + 4*((int)tp[0]) + ((int)tp[width-1]);
        acc += 2*((int)tp[width]);
        acc += 255 * 5;
        acc /= 16;
        *dp++ = (unsigned char)acc;
        tp++;
        for(i = 1; i < height-1; i++) {
            acc = 2*((int)tp[-width]) + ((int)tp[-(width-1)]);
            acc += 4*((int)tp[0]) + 2*((int)tp[1]);
            acc += 2*((int)tp[width]) + ((int)tp[width+1]);
            acc += 255 * 3;
            acc /= 16;
            *dp++ = (unsigned char)acc;
            tp++;
            for(j = 1; j < width-1; j++) {
                acc = ((int)tp[-(width+1)]) + 2*((int)tp[-width]);
                acc += ((int)tp[-(width-1)]) + 2*((int)tp[-1]);
                acc += 4*((int)tp[0]) + 2*((int)tp[1]) + ((int)tp[width-1]);
                acc += 2*((int)tp[width]) + ((int)tp[width+1]);
                acc /= 16;
                *dp++ = (unsigned char)acc;
                tp++;
            }
            acc = ((int)tp[-(width+1)]) + 2*((int)tp[-width]);
            acc += 2*((int)tp[-1])  + 4*((int)tp[0]) + ((int)tp[width-1]);
            acc += 2*((int)tp[width]);
            acc += 255 * 3;
            acc /= 16;
            *dp++ = (unsigned char)acc;
            tp++;
        }
        acc = 2*((int)tp[-width]) + ((int)tp[-(width-1)]);
        acc += 4*((int)tp[0]) + 2*((int)tp[1]);
        acc += 255 * 5;
        acc /= 16;
        *dp++ = (unsigned char)acc;
        tp++;
        for(j = 1; j < width-1; j++) {
            acc = ((int)tp[-(width+1)]) + 2*((int)tp[-width]);
            acc += ((int)tp[-(width-1)]) + 2*((int)tp[-1]);
            acc += 4*((int)tp[0]) + 2*((int)tp[1]);
            acc += 255 * 3;
            acc /= 16;
            *dp++ = (unsigned char)acc;
            tp++;
        }
        acc = ((int)tp[-(width+1)]) + 2*((int)tp[-width]);
        acc += 2*((int)tp[-1])  + 4*((int)tp[0]);
        acc += 255 * 5;
        acc /= 16;
        *dp++ = (unsigned char)acc;
        tp++;
    }
    if (k) {
        tp = temp2;
    } else {
        tp = temp;
    } 
    dp = data;
    memset(hist, 0, sizeof(hist));
    for(i = 0; i < height; i++) {
        for(j = 0; j < width; j++) {
            acc = ((int)*dp) - ((int)*tp);
            if (((acc < 0)?-acc:acc) >= thresh) {
                acc *= amount;
                acc /= 100;
                acc += ((int)*dp);
                if (acc < 0)
                    acc = 0;
                if (acc > 255)
                    acc = 255;
                *dp = (unsigned char)acc;
            }
            hist[(int)*dp++]++;
            tp++;
         }
    }
    for (i = 0; i < 128; i++) {
        if (hist[i] != 0) {
            min = i;
            break;
       }
    }
    for (i = 255; i > 128; i--) {
        if (hist[i] != 0) {
             max = i;
             break;
        }
    }
    scale = 256.0 / ((float)(max - min));
    for (i = 0; i < 256; i++) {
        int k = (int)(((float)(i - min))*scale);
        if (k < 0)
            k = 0;
        if (k > 255)
            k = 255;
        map[i] = k;
    }

    memset(hist, 0, sizeof(hist));
    dp = data;
    for (i = row_width * height; i > 0; i--, dp++) 
         hist[(int)(*dp = map[(int)*dp])]++;
    delete[] temp;
    delete[] temp2;
}

// Enhance the constrast of an image.
void
Image::contrast(int angle, int bright)
{
    unsigned char map[256];
    float         tan_angle;
    float         tan_range;
    unsigned char    *dp;
    int           i;

    if (bpp != 8)
        return;
    tan_angle = tan(((float)(angle)) * (M_PI/180.0));
    tan_range = 128.0 * tan_angle;
    if (verbose)
        fprintf(stderr, "    contrast %d %d\n", angle, bright);
    for (i = 0; i < 256; i++) {
         int t;
         if (i < (int)(128.0 + tan_range) && i > (int)(128.0-tan_range))
               map[i] = 128 + (int)((float)(i-128)/tan_angle);
         else if (i >= (int)(128.0 + tan_range))
               map[i] = 255;
         else
               map[i] = 0;
         t = map[i] + bright;
         if (t > 255)
             t = 255;
         else if (t < 0)
             t = 0;
         map[i] = t;
    }
         
    dp = data;
    memset(hist, 0, sizeof(hist));
    for (i = row_width * height; i > 0; i--, dp++) 
         hist[(int)(*dp = map[(int)*dp])]++;
}

// Create border around image.
void
Image::boardFill()
{
    int             i, j, k;
    unsigned char   row_buffer[width];
    unsigned char   finished[width + 1];
    unsigned char   *dp = data;
    int             max;    // max value to replace.
    int             pix;    // replacement pixel

    pix = (1 << bpp) - 1;
    max = pix >> 1;
    memset(finished, 0, width);
    // Scan from top down 
    for (i = 0; i< height ; i++) {
        // Explode row 
        unpackrow(width, dp, row_buffer);
        // Scan toward center 
        for (j = 0; j < width/2; j++) {
            if (row_buffer[j] <= max) {
                row_buffer[j] = pix;
                finished[j+1] = 0;  // Check next one over
            } else {
                int l;
                // Check next 10 pixels
                for (l = 0; l < 10; l++) 
                    if (row_buffer[j+l] <= max)
                       break;
                if (l == 10)        // Nothing to do, done with row.
                    break;
            }
        }

        if (j == 0) // Stop at 1 on back scan
            j++;
        // Back scan to where we finished.
        for (k = width-1; k > j && row_buffer[k] <= max; k++) {
            if (row_buffer[j] <= max) {
                row_buffer[j] = pix;
                finished[k-1] = 0;  // Check next one over
            } else {
                int l;
                // Check next 10 pixels
                for (l = 0; l < 10; l++) 
                    if (row_buffer[k-l] <= max)
                       break;
                if (l == 10)        // Nothing to do, done with row.
                    break;
            }
        }
        // Scan top 
        for (; j < k; j++) {
            // Stop after we have not found anything in 10 rows.
            if (finished[j] < 10) {
               if (row_buffer[j] <= max) 
                  row_buffer[j] = pix;
               else
                  finished[j]++;
            }
        }
        // Put it back.
        packrow(width, row_buffer, dp);
        dp += row_width;
    }
    memset(finished, 0, width);     // Reset finished arrary
    // Scan from bottom to top
    for (; i > 0 ; i--) {
        int did = 0;
        dp -= row_width;
        // Explode row 
        unpackrow(width, dp, row_buffer);
        // Scan toward center 
        for (j = 0; j < width && row_buffer[j] <= max; j++) {
            row_buffer[j] = pix;
            finished[j+1] = 0;      // Check next one over
            did++;
        }
        if (j == 0) // Stop at 1 on back scan
            j++;
        // Back scan to where we finished.
        for (k = width-1; k > j && row_buffer[k] <= max; k++) {
            row_buffer[k] = pix;
            finished[k-1] = 0;      // Check previous one
            did++;
        }
        // Scan top 
        for (; j < k; j++) {
            if (finished[j] < 10) {
               if (row_buffer[j] <= max) {
                  row_buffer[j] = pix;
                  did++;
               } else {
                  finished[j]++;
               }
            }
        }
        // Done if we made no changes
        if (did == 0)
            break;
        // Put it back.
        packrow(width, row_buffer, dp);
    }
}
