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

// Handle collections of PDF objects.

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#include <stdio.h>
#if HAVE_STRING_H
# if !STDC_HEADERS && HAVE_MEMORY_H
#  include <memory.h>
# endif
# include <string.h>
#endif
#if HAVE_STRINGS_H
# include <strings.h>
#endif
#if HAVE_UNISTD_H
# include <unistd.h>
#endif

#ifndef _OBJ_H_
#define _OBJ_H_
#define HDR     "%PDF-1.3\n\n%\305\324\234\234\n\n"

class Obj;
class ObjList;
class Pages;
class Page;
class Section;
class Sections;
class Stream;
class PDFfile;
class Resource;
class ResList;

// List of PDF file objects.
class ObjList {
friend class PDFfile;
public:
        int     count;          // Number of objects in this list.
        Obj     *obj;           // List of objects are an object themselves.
protected:
        struct objlink {
               Obj             *obj;
               struct objlink  *next;
               struct objlink  *prev;
        }      *first, *last;

public:

        // Create object list.
        ObjList(Obj *obj = 0) : count(0), obj(obj), first(0), last(0) {
        };

        ~ObjList() {
            struct objlink      *l = first;
            while(l != NULL) {
                struct objlink  *p = l->next;
                delete l;
                l = p;
            }
        };

        // Check if object is on this list.
        int in(Obj *obj);

        // Copy objects to new list.
        void copyTo(ObjList *ol);

        // Check if this is same as this list.
        int same(ObjList *ol);

        // Add image reference.
        void refImg();

        Obj *add(Obj *nobj = NULL);

        void ref(const char *title = NULL);
        
        void putXref(PDFfile *f);

        void put(const char *title = NULL);

        void putArray(const char *title = NULL);

        void open(const char *title = NULL);

        void close();

        void clear(); 
};

// Section resources.
class   Resource {
public:
        Obj     *obj;
        Obj     *font1;
        Obj     *font2;
        ObjList imgs;

        Resource() : obj(0), font1(0), font2(0) {};

        ~Resource() {};

        int same(Resource *res);
        
        void addFont1(Obj *font) { font1 = font;}
        void addFont2(Obj *font) { font2 = font;}

        void addImage(Obj *img) { imgs.add(img);}

        void clear() { font1 = font2 = obj = 0;
                imgs.clear(); }

        void put();
};

// List of resources.
class ResList {
protected:
        struct reslink {
                Resource        res;
                struct reslink  *next;
                struct reslink  *prev;
        }       *first, *last;

public:

        ResList() : first(0), last(0) {
        };

        ~ResList() {
            struct reslink      *l = first;
            while(l != NULL) {
                struct reslink  *p = l->next;
                delete l;
                l = p;
            }
        };

        Obj *find(Resource *res, PDFfile *f);

};

// Individual PDF objects.
class Obj {
public:
        int     number;
private:
        int     offset;
        int     array;
        int     fobj;
        Obj     *next;
        PDFfile *file;  // File printing on.

public:
        Obj(PDFfile *file, int number, int array = 0) : 
                number(number), offset(0), array(array),
                fobj(0), next(0), file(file) {
        };

        Obj *newObj(int array = 0);

        void open(const char *type = NULL);

        void close();

        void ref(const char *title = NULL);

        void put(const char *str);

        void putN(const char *str);

        void put(const char *str, const int v);

        void put(const char *name, const char *v);

        void put(const char *name, time_t t);

        void put(const char c);

        void put(int v);

        void putdata(const char *data, const int len);

        int get_offset() { return offset; }
};


#endif
