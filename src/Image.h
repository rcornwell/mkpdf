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

// Basic image processing functions.
#include "Obj.h"

#ifndef _IMAGE_H_
#define _IMAGE_H_
#include <libxml/xmlIO.h>
        
class   Image {
public:
        int                     width;
        int                     row_width;
        int                     height;
        int                     bpp;
        unsigned char           *data;
        unsigned long int       hist[256];


        Image() : width(0), height(0), bpp(0), data(0) {};

        ~Image() { delete data; };

        int open(xmlChar *name);

        int d_width() { return (width * 1000) / 4166; };

        int d_height() { return (height * 1000) / 4166; };

        Obj *save(Stream *strm);

        void avg(int value);

        void thresh(int value);

        void flip();

        void reverse();
        
        void transpose();

        void rotater90();

        void rotatel90();

        void rotate180();

        void unsharp(int amount, int radius, int thresh);

        void contrast(int angle, int bright);

        void boardFill();

private:
        void unpackrow(int width, unsigned char *in, unsigned char *out);

        void packrow(int width, unsigned char *in, unsigned char *out);
};
        
#endif
