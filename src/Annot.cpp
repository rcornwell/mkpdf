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

// Handle embedding of files into a PDF document.

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h> 
#include <sys/stat.h>

#include "Obj.h"
#include "PDFFile.h"
#include "Stream.h"
#include "Annot.h"


extern int      verbose;

//
// Load a file into the generated PDF file.
//
// file:  currently building PDF file.
// name:  name of file to include.
// ftype: type name to be displayed with file.
// mode:  mode of file, non-zero is binary, zero is ascii.
//
void
Annot::load(PDFfile *file, const char *name, const char *ftype, int mode)
{
    FILE    *f;
    char    buffer[1024];
    int     len;
    Stream  *fs;
    struct stat st;

    f = fopen(name, "r");
    if (!f) {
        fprintf(stderr, "Unable to include file %s\n", name);
        return;
    }

    if (verbose) 
        fprintf(stderr, "Including file %s (%s) ", name, ftype);

    // Create a sub stream.
    fs = file->newStream();
    fs->open("EmbeddedFile");

    // Set mode.
    if(mode) 
       fs->put("/Subtype/Application#2Foctet-stream"); 
    else 
       fs->put("/Subtype/Text#2Fplain#20charset=us-ascii");
    // Slurp file in in chunks
    if (mode) {
        while((len = fread(buffer, 1, sizeof(buffer), f)) > 0) 
            fs->appendData(buffer, len);
    } else { 
        while(fgets(buffer, sizeof(buffer), f) != NULL) {
            char    *p = &buffer[strlen(buffer)-1];
            if (*p == '\n')
                p--;
            if (*p != '\r')
                *++p = '\r';
            *++p = '\n';
            *++p = '\0';
            fs->appendCmd(buffer);
        }
    }
    // Make sure all data is pushed to stream.
    fs->flush();
    // Create description of object in PDF file.
    fs->put("/Params <<");
    fs->put("Size", (int)fs->size);
    if (verbose)
        fprintf(stderr, "%d bytes\n", fs->size);
    stat(name, &st);
    fs->put("CreationDate", st.st_mtime);
    fs->put(">> ");
    fs->close();
    // Append to file.
    obj->open("Filespec");
    obj->put("F", name);
    obj->put("/EF<<");
    fs->ref("F");
    obj->put(">> ");
    obj->close();
    delete fs;

    if (ftype) {
        type = new char[strlen(ftype)+1];
        strcpy(type, ftype);
    }
}

//
// Put the Annotation to the file.
void
Annot::put(Obj *o)
{
    char    buffer[1000];
   
    o->open("Annot");
    o->put("/Subtype/FileAttachment/F 0/Name/Tag");
    sprintf(buffer, "Rect [%d %d %d %d]/T", x, y, x+20, y+20);
    o->put(buffer, id);
    strcpy(buffer, id);
    if (type) {
        strcat(buffer, " ");
        strcat(buffer, type);
    }
    o->put("Contents", buffer);
    obj->ref("FS");
    o->close();
}

//
// Make a new annotation.
Annot
*Annots::newAnnot(Obj *o)
{
    Annot   *a;

    a = new Annot(o);
    a->next = list;
    list = a;
    return a;
}

//
// Set place where annotation will appear.
void
Annots::ref(char *name, int x, int y)
{
    Annot   *a = list;

    while(a != 0) {
        if (strcmp(a->id, name) == 0) {
            a->x = x;
            a->y = y;
            a->ref = 1;
            return;
        }
        a = a->next;
    }
}

//
// Print out the Annotations.
void
Annots::put(PDFfile *file)
{
    Annot   *a = list;
    Annot   *b = 0;
    Annot   *c;

    while(a != 0) {
        if (a->ref) {
            Obj     *o = file->newObj();
            a->put(o);
            file->addAnnot(o);
            c = a->next;
            delete a;
        } else {
            c = a->next;
            a->next = b;
            b = a;
        }
        a = c;
    }
    list = b;
}
