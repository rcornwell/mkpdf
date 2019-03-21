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

// Master control object for PDF file.
#include "Obj.h"
#include "Image.h"
#include "Annot.h"

#ifndef _PDFFILE_H_
#define _PDFFILE_H_


class PDFfile {
        char            *name;
        FILE            *file;
        int             offset;
        Obj             *first;
        Obj             *last;
        Obj             *info;  
        Annots           annots;
        ObjList         objs;
        int             nextnum;
        Sections        *sects;
        Section         *cur_sect;
        Pages           *port_pages;
        Pages           *land_pages;
        Page            *cur_page;
public:
        Obj             *font1, *font2;
        ResList res_cache;

        PDFfile() {
            name = 0;
            file = 0;
            offset = 0;
            nextnum = 0;
            sects = 0;
            cur_sect = 0;
            port_pages = 0;
            land_pages = 0;
            cur_page = 0;
        }

        ~PDFfile() {
            if (file)
                fclose(file);
            delete[] name;
        }

        Obj     *newObj(int array = 0);

        int open(char *fname);

        void addSection(char *title = 0);

        void addAnnot(Obj *o);

        Annot *newAnnot() { return annots.newAnnot(newObj());}

        Page *newPage(int land);

        Stream *newStream();

        void title(const char *str);

        void put(const char *str);

        void putN(const char *str);

        void put(const char *str, int v);

        void put(const char *str, const char *v);

        void put(const char *name, const time_t t);

        void put(const char c);

        void put(int v);

        void putdata(const char *data, const int len);
            
        void close();
        
        int get_offset() { return offset; }

        void    convertFile(char *name, int lpp, int land);

        void    convertText(char *name);
        
        void    convertImage(char *name, Image *img, int land);
};


#endif
