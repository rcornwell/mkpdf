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

// PDF objects.

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h> 

#include "Obj.h"
#include "Stream.h"
#include "Page.h"
#include "PDFFile.h"

// Add an object to list of objects in PDF file.
//
// nobj: New object on the list.
//
// Returns the object to simplify other coding.
Obj
*ObjList::add(Obj *nobj)
{
    struct objlink      *n = new struct objlink;

    if (nobj == NULL)
        n->obj = obj->newObj();
    else
        n->obj = nobj;
    n->next = 0;
    if (first == 0) {
        first = last = n;
        n->prev = 0;
    } else {
        last->next = n;
        n->prev = last;
        last = n;
    }
    count++;
    return n->obj;
}


// Generate cross reference section of PDF file.
// For each object report it's byte offset in the file.
void
ObjList::putXref(PDFfile *pdf)
{
    struct objlink      *l;
    char                buffer[40];
    char                *p;
    int                 n;
    int                 lnk = 0;

    for(l = first; l != NULL; l = l->next) {
        strcpy(buffer, "0000000000 00000 n \n");
        n = l->obj->get_offset();
        if (n == 0) {
            n = lnk;
            lnk = l->obj->number;
            buffer[17] = 'f';
        }
        p = &buffer[9];
        while(n > 0 && p != buffer) {
            *p-- = (n % 10) + '0';
            n /= 10;
        }
        pdf->put(buffer);
    }
}

// Cross reference one object to another.
void
ObjList::ref(const char *title)
{
    obj->ref(title);
}

// Print the list of objects and number of items.
void
ObjList::put(const char *title)
{
    putArray(title);
    obj->put("Count", count);
}

// Print out all objects in a file.
void
ObjList::putArray(const char *title)
{
    struct objlink      *l;

    if (count == 0) 
        return;
    if (title != NULL) 
        obj->putN(title);
    obj->put(" [");
    for (l = first; l != 0; l = l->next) 
        l->obj->ref();
    obj->put(" ]");
}

// Open a list of objects.
void
ObjList::open(const char *title)
{
    obj->open(title);
}

// Close the list of objects.
void
ObjList::close()
{ 
    obj->close();
}

// Remove all objects in list.
void
ObjList::clear()
{
    struct objlink      *l = first;

    while(l != NULL) {
        struct objlink  *p = l->next;
        delete l;
        l = p;
    }
    count = 0;
    first = 0;
    last = 0;
}

// Check if object is in list.
//
// Return true if object exists in the list.
int
ObjList::in(Obj *obj)
{
    struct objlink      *l;

    for (l = first; l != 0; l = l->next) {
        if (l->obj == obj)
           return 1;
    }
    return 0;
}

// Copy objects to new list.
void
ObjList::copyTo(ObjList *nl)
{
    struct objlink      *l;

    for (l = first; l != 0; l = l->next) 
        nl->add(l->obj);
}

// Check if object list is same list of objects.
int
ObjList::same(ObjList *ol)
{
    struct objlink      *l;

    if (count != ol->count)
        return 0;
    for (l = first; l != 0; l = l->next) {
        if (!ol->in(l->obj))
           return 0;
    }
    return 1;
}

// Put out a list of objects on this list.
void
ObjList::refImg()
{
     char       buffer[100];
     struct objlink     *l;

     for (l = first; l != 0; l = l->next) {
        sprintf(buffer, "Im%d", l->obj->number);
        l->obj->ref(buffer);
    }
}

// Check if a resource is the same.
int
Resource::same(Resource *res)
{
     if (font1 != res->font1)
        return 0;
     if (font2 != res->font2)
        return 0;
     if (imgs.same(&res->imgs))
        return 1;
     return 0;
}

// Print resources into PDF file.
void
Resource::put()
{
     obj->open();
     if (font1 != 0 || font2 != 0) {
         obj->put("/Font<<");
         if (font1)
             font1->ref("FF");
         if (font2)
             font2->ref("FS");
         obj->put(">>");
     }
     if (imgs.count != 0) {
         obj->put("/XObject<<");
         imgs.refImg();
         obj->put(">>");
     }
     obj->put("/ProcSet[/PDF");
     if (font1 || font2)
         obj->put("/Text");
     if (imgs.count != 0) 
         obj->put("/ImageB");
     obj->put("]");
     obj->close();
}

Obj
*ResList::find(Resource *res, PDFfile *f)
{
    struct reslink     *l;

    for (l = first; l != 0; l = l->next) {
         if (l->res.same(res))
            return l->res.obj;
    }
    l = new struct reslink;
    l->res.obj = f->newObj(1);
    l->res.font1 = res->font1;
    l->res.font2 = res->font2;
    res->imgs.copyTo(&l->res.imgs);
    l->next = 0;
    if (first == 0) {
        first = last = l;
        l->prev = 0;
    } else {
        last->next = l;
        l->prev = last;
        last = l;
    }
    l->res.put();
    return l->res.obj;
}

// Create a new object
Obj
*Obj::newObj(int array)
{
    return file->newObj(array);
}

// Create a new object.
void
Obj::open(const char *type)
{
    offset = file->get_offset();
    put(number);
    put(" 0 obj <<");
    if (!array && type != NULL) {
        put("/Type");
        putN(type);
    }
}

// Finished creating an object, close it off.
void
Obj::close()
{
    file->put(">>endobj\n");
}

// Refer to an object by name.
void
Obj::ref(const char *title)
{
    if (title) 
        putN(title);
    put(' ');
    put(number);
    put(" 0 R");
}

// Put text into file.
void
Obj::put(const char *str)
{
    file->put(str);
}

void
Obj::putN(const char *str)
{ 
    file->putN(str);
}

void
Obj::put(const char *str, int v)
{ 
    file->put(str, v);
}

void
Obj::put(const char *name, const char *v)
{ 
    file->put(name, v);
}

void
Obj::put(const char *name, time_t t)
{ 
    file->put(name, t);
}

void
Obj::put(const char c)
{
    file->put(c);
}

void
Obj::put(int v)
{
    file->put(v);
}

void
Obj::putdata(const char *data, const int len)
{
    file->putdata(data,len);
}

