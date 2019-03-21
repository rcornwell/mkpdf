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

// Handle embedded files.

#if HAVE_STRING_H
# if !STDC_HEADERS && HAVE_MEMORY_H
#  include <memory.h>
# endif
# include <string.h>
#endif
#if HAVE_STRINGS_H
# include <strings.h>
#endif
#include "Obj.h"

#ifndef _ANNOT_H_
#define _ANNOT_H_

class   PDFfile;
class   Annots;

class   Annot  {
friend class Annots;
        Obj             *obj;
        char            *id;
        int             x;
        int             y;
        int             ref;
        char            *type;
        Annot           *next;
public:

        // Create an Annotation object.
        Annot(Obj *o) : obj(o), id(0), x(0), y(0), ref(0), type(0), next(0) {};
        
        ~Annot() { delete[] id; delete[] type; obj = 0; next = 0; }

        // Give it a name.
        void setid(char *name) {
             id = new char[strlen(name)+1];
             strcpy(id, name);
        }

        // Load a file into it.
        void load(PDFfile *file, const char *name, const char *ftype, int mode);

        // Put this annotation into file.
        void put(Obj *o);
};


// Handle list of annotations in this file.     
class   Annots  {
        Annot   *list;
public:
        Annots() : list(0) {};

        ~Annots() { }

        void ref(char *name, int x, int y);

        // Create a new annotation.
        Annot *newAnnot(Obj *o);

        // Dump annotations to file.
        void put(PDFfile *file);
};
        
#endif
