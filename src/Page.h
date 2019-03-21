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

// PDF page objects.
#include "Obj.h"

#ifndef _PAGE_H_
#define _PAGE_H_


// List of pages that make up a section.
class   Pages   : public ObjList {
public:
        int     land;


        Pages(Obj *o, int land) : ObjList(o), land(land) {}

        void printPages(Obj *parent = 0) {
             obj->open("Pages");
             if (parent != 0) 
                 parent->ref("Parent");
             if (land)
                 obj->put("/MediaBox[0 0 792 612]");
             else
                 obj->put("/MediaBox[0 0 612 792]");
             obj->put("/Rotate 0\n");
             put("Kids");
             obj->close();
        }
};
        
class   Section;

// Information about a page.
class   Page {
friend  class Section;
        int     number;
        Obj     *obj;
        Pages   *parent;
        Obj     *res;
        Obj     *cont;
        Page    *next;
        Page    *prev;
        ObjList annots;

public:
        Page(Obj *obj, int number=1) : number(number), obj(obj) {
            annots.obj = obj;
        }

        ~Page() { };

        void Set_parent(Pages *pgs) { 
                parent = pgs; 
                parent->add(obj);
        }

        void resource(Obj *o) { res = o; }

        void content(Obj *obj) { cont = obj; }

        void addAnnot(Obj *obj) {
             annots.add(obj);
        }

        void printDest() {
             obj->ref("Dest [");
             obj->put(" /XYZ null null null]");
             obj->put("Title (Page", number);
             obj->put(")");
        }

        void open() { obj->open("Page"); }

        void close() {
             parent->ref("Parent");
             res->ref("Resources");
             cont->ref("Contents");
             if (annots.count > 0)
                 annots.putArray("Annots");
             obj->close();
        }
};

class Sections;
// Section of PDF file.
class Section {
friend  class Sections;
        char    *title;
        Obj     *obj;
        int     count;
        int     pgnum;
        Page    *first;
        Page    *last;
        Section *next;
        Section *prev;
        Obj     *parent;
        
public:
        Section(char *text, Obj *obj) : obj(obj), count(0), 
            pgnum(1), first(0), last(0), next(0), prev(0) {
            if (text != 0) {
                title = new char[strlen(text)+1];
                strcpy(title, text);
            } else
                title = 0;
        }

        ~Section() {
                delete[] title;
        }

        void addPage(Page *pg) {
             pg->number = pgnum++;
             pg->next = 0;
             if (first == 0) {
                 first = pg;
                 last = pg;
                 pg->prev = 0;
             } else {
                 last->next = pg;
                 pg->prev = last;
                 last = pg;
             }
             count++;
        }

        void ref(const char *str = 0) { obj->ref(str); }

        Obj *put(Obj *par) {
             Obj       *pg, *ppg, *fs;
             Page      *cur_pg;

             ppg = NULL;
             if (title == 0)
                 pg = obj;
             else
                 pg = obj->newObj(1);
             fs = pg;
             for(cur_pg = first; cur_pg != 0; cur_pg = cur_pg->next) {
                 pg->open();
                 cur_pg->printDest();
                 if (title == 0)
                     par->ref("Parent");
                 else
                     obj->ref("Parent");
                 if (ppg != NULL)
                     ppg->ref("Prev");
                 ppg = pg;
                 if (cur_pg->next != NULL) {
                     pg = obj->newObj(1);
                     pg->ref("Next");
                 }
                 ppg->close();
             }
             if (title != 0) {
                 obj->open();
                 obj->put("Title", title);
                 par->ref("Parent");
                 if (prev) 
                     prev->ref("Prev");
                 if (next)
                     next->ref("Next");
                 fs->ref("First");
                 pg->ref("Last");
                 obj->put("Count", count);
                 obj->close();
                 return 0;
            }
            return pg;
        }
};

// PDF file is a collection of sections.
class   Sections {
        Obj     *obj;
        int     count;
        Section *first;
        Section *last;

public:    
        Sections(Obj *obj) : obj(obj), count(0), first(0), last(0) {}

        ~Sections() {};

        void add(Section *sect) {
             sect->parent = obj;
             sect->next = 0;
             if (first == 0) {
                 first = sect;
                 last = sect;
                 sect->prev = sect;
             } else {
                 last->next = sect;
                 sect->prev = last;
                 last = sect;
             }
             count++;
        }

        void put() {
             Section    *sect = first;
             Obj        *lpg = 0;

             for (sect = first; sect != 0;  sect = sect->next) {
                  lpg = sect->put(obj);
                  if (lpg != 0)
                     count += sect->count; 
             }
             obj->open("Outlines");
             first->ref("First");
             if (last->title == 0)
                 lpg->ref("Last");
             else
                 last->ref("Last");
             obj->put("Count", count);
             obj->close();
        }

        void ref(const char *title = 0) { obj->ref(title); }
};

#endif
