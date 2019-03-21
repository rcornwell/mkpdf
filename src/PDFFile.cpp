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

// PDF document main collector.
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h> 
#include <png.h>
#include <sys/stat.h>

#include "Obj.h"
#include "PDFFile.h"
#include "Stream.h"
#include "Page.h"
#include "Image.h"
#include "Annot.h"


extern int      verbose;

// Create new object for this file.
Obj
*PDFfile::newObj(int array)
{
    Obj     *n = new Obj(this, ++nextnum, array); 

    objs.add(n);
    return n;
}

// Open the file for generation.
int
PDFfile::open(char *fname)
{
    name = new char[strlen(fname)+1];
    strcpy(name, fname);
    file = fopen(name, "w");
    if (file == 0)
        return 1;
    put(HDR);
    return 0;
}

// Add in a section.
void
PDFfile::addSection(char *title)
{
     Obj        *t;
     t = newObj(0);
     cur_sect = new Section(title, t);
     sects->add(cur_sect);
     cur_page = 0;
}

// Add in an annotation.
void
PDFfile::addAnnot(Obj *o)
{
     if (cur_page) 
        cur_page->addAnnot(o);
}

// Add in a new page.
Page
*PDFfile::newPage(int land)
{
     Obj        *p;

     if (cur_sect == 0)
         addSection(NULL);
     p = newObj(0);
     cur_page = new Page(p);
     if (land) {
         if (land_pages == 0) 
             land_pages = new Pages(newObj(0), 1);
         cur_page->Set_parent(land_pages);
     } else {
         if (port_pages == 0) 
             port_pages = new Pages(newObj(0), 0);
         cur_page->Set_parent(port_pages);
     }
     cur_sect->addPage(cur_page);
     return cur_page;
}

// New stream object.
Stream
*PDFfile::newStream()
{
     Obj        *s;

     s = newObj(0);
     return new Stream(s);
}

// Set title of PDF file.
void
PDFfile::title(const char *str)
{
     time_t     tx;
     Obj        *t;

     info = newObj(1);
     info->open();
     put("/Producer(toPDF)");
     put("Title", str);
     time(&tx);
     put("CreationDate", tx);
     info->close();                                                     
     t = newObj(0);
     sects = new Sections(t);
}

// Put a character onto PDF file.
void
PDFfile::put(const char c)
{
    fputc(c, file);
    offset++;
}

// Put string into PDF file.
void
PDFfile::put(const char *str)
{
    if (str == NULL)
        return;
    fputs(str, file);
    offset += strlen(str);
}

// Put /string onto PDF file.
void
PDFfile::putN(const char *str)
{
    if (str == NULL)
        return;
    fputc('/', file);
    fputs(str, file);
    offset += strlen(str)+1;
}

// Put a /string number onto PDF file
void
PDFfile::put(const char *str, int v)
{
    putN(str);
    put(' ');
    put(v);
}

// Put a /string string onto PDF file.
void
PDFfile::put(const char *str, const char *v)
{
    putN(str);
    put(' ');
    put('(');
    put(v);
    put(')');
}

// Put /string time onot PDF file
void
PDFfile::put(const char *name, const time_t t)
{
     char       buffer[40];
     char       *p;
     struct tm  *tm;

     tm = localtime(&t);
     strftime(buffer, sizeof(buffer), "D:%Y%m%d%H%M%S%z '", tm);
     p = strrchr(buffer, '\'');
     p[-1] = p[-2];
     p[-2] = p[-3];
     p[-3] = '\'';
     put(name, buffer);
}

// Put a number onto PDF file.
void
PDFfile::put(int v)
{
    char        buffer[30];
    char        *p;

    p = buffer;
    *p++ = '\0';
    if (v < 0) {
        *p++ = '-';
        v = -v;
    }
    do {
        *p++ = (v % 10) + '0';
        v /= 10;
    } while (v != 0);
    while(*--p != '\0') {
        fputc(*p, file);
        offset++;
    }
}

// Put data onto PDF file.
void
PDFfile::putdata(const char *data, const int len)
{
    fwrite(data, 1, len, file);
    offset += len;
}
    

// Finalize a PDF file.
void
PDFfile::close()
{
    Obj         *o;
    Obj         *cat;
    ObjList     *pages;
    int         xrefoffset;

    // First place all pages into file.
    if (port_pages != 0 && land_pages != 0) {
        o = newObj(0);
        pages = new ObjList(o);
        if (port_pages->count != 0) {
            port_pages->printPages(o);
            pages->add(port_pages->obj);
        }
        if (land_pages->count != 0) {
            land_pages->printPages(o);
            pages->add(land_pages->obj);
        }
        pages->count = port_pages->count + land_pages->count;
        pages->open("Pages");
        pages->put("Kids");
        pages->close();
    } else if (port_pages != 0) {
        port_pages->printPages();
        pages = port_pages;
    } else {
        land_pages->printPages();
        pages = land_pages;
    }

    // Create the Outline.
    sects->put();
    cat = newObj(0);
    cat->open("Catalog");
    sects->ref("Outlines");
    pages->ref("Pages");
    put("/PageMode/UseOutlines");
    cat->close();

    // Add in the cross reference.
    xrefoffset = offset;
    put("xref\n0 ");
    put(nextnum+1);
    put("\n");
    put("0000000000 65535 f \n");
    objs.putXref(this);
    put("trailer\n<<");
    put("Size", nextnum+1);
    cat->ref("Root");
    if (info)
        info->ref("Info"); 
    put(">>\nstartxref\n");
    put(xrefoffset);
    put("\n%%EOF\n");
}


//
// Convert listing file into pages. Honoring carriage control characters.
//
void
PDFfile::convertFile(char *name, int lpp, int land)
{
    char      buffer[1000];
    int       line;
    int       blank_title;
    Page      *page;
    Stream    *strm;
    Resource  res;
    FILE      *f;
    char      *p;
    int       first = 1;

    line = 0;
    f = fopen(name, "r");
    if (f == NULL) {
        fprintf(stderr, "Unable to open %s\n", name);
        return;
    }
    if (verbose) 
        fprintf(stderr, "Including %s listing %s\n",
            (land)?"landscape": "portrat", name);
    res.addFont1(font1);
    strm = 0;
    page = 0;
    blank_title = 0;                // Flag for blank title line.
    while(fgets(buffer, sizeof(buffer), f)) { // Grab a line
        // Strip off newline
        p = strrchr(buffer, '\n');
        if (p)
            *p = '\0';
        // Strip off return
        p = strrchr(buffer, '\r');
        if (p)
            *p = '\0';
        if (buffer[0] == '\0')
            break;
        // Clear trailing blanks
        p = &buffer[strlen(buffer)-1];
        while(*p == ' ' && p != &buffer[0]) p--;
        *++p = '\0';
        if (line > lpp)     // Force new page if over lines per page
            buffer[0] = '1';
        if (blank_title) {
            if (strm == 0) {
                strm = newStream();
                page = newPage(land);
                strm->appendCmd("BT\n/FF 10 Tf ");
                if (land) 
                    strm->appendCmd("10 TL 1 0 0 1 10 600 Tm\n");
                else
                    strm->appendCmd("12 TL 1 0 0 1 10 752 Tm\n");
            }
            strm->appendCmd("T*\n");
            blank_title = 0;
        }
        // Flush out old page if new page 
        if (buffer[0] == '1' && strm != 0) {
            page->content(strm->obj);
            page->resource(res_cache.find(&res, this));
            strm->appendCmd("ET\n");
            strm->close();
            page->open();
            page->close();
            delete strm;
            strm = 0;
        }
        if (buffer[0] == '1' && buffer[1] == '\0') {
            blank_title = !first;   // Skip first blank title
        } else {
            // Allocate a new stream
            if (strm == 0) {
                strm = newStream();
                page = newPage(land);
                strm->appendCmd("BT\n /FF 10 Tf ");
                if (land) 
                     strm->appendCmd("10 TL 1 0 0 1 10 600 Tm\n");
                else
                     strm->appendCmd("12 TL 1 0 0 1 10 752 Tm\n");
                line = 0;
            }
            // Decide how to process first char.
            switch(buffer[0]) {
            case '\0':  break;
            case '2':   strm->appendCmd("T*\n"); 
                        line++;
            case '0':   strm->appendCmd("T*\n"); 
                        line++;
            case '1':   
            default:
                        if (buffer[1] == '\0') {
                            strm->appendCmd("T*\n");
                        } else {
                            strm->appendCmd("T* (");
                            strm->appendString(&buffer[1]);
                            strm->appendCmd(") Tj\n");
                        }
                        line++;
                        break;
            }
            first = 0;
        }
    }
    // Flush out if anything left in buffer.
    if (strm != 0) {
        page->content(strm->obj);
        page->resource(res_cache.find(&res, this));
        strm->appendCmd("ET\n");
        strm->close();
        page->open();
        page->close();
        delete strm;
    }
    fclose(f);
    return;
}

struct  unline {
        int     sx;
        int     sy;
        int     ex;
        int     ey;
        int     num;
        struct unline *next;
};

// Convert a text file into PDF with limited formating codes.
void
PDFfile::convertText(char *name)
{
    char    buffer[1000];
    char    out[1000];
    char    *p, *q;
    int     offset = 0;
    int     pos;
    int     line;
    struct  unline  *uline = 0, *ulst;
    struct  unline  *image, *ilst;
    FILE    *f;
    Stream  *strm;
    Resource res;
    Page    *page;
    int     spacing = 12;
    char    collect = 0;
    int     i;

    ulst = 0;
    ilst = 0;
    f = fopen(name, "r");
    if (f == NULL) {
        fprintf(stderr, "Unable to open text %s\n", name);
        return;
    }
    if (verbose)
        fprintf(stderr, "Processing text %s\n", name);
    strm = newStream();
    page = newPage(0);
    page->content(strm->obj);
    res.addFont1(font1);
    strm->appendCmd("BT\n/FF 10 Tf 12 TL 1 0 0 1 10 752 Tm");
    line = 0;
    pos = 0;
    while(!feof(f)) {
        if (fgets(buffer, sizeof(buffer), f) == NULL)
            break;
        p = strrchr(buffer, '\n');
        if (p)
            *p = '\0';
        strm->appendCmd("\nT* ");
        line+=spacing;
        pos = 0;
        /* Clear trailing blanks */
        p = &buffer[strlen(buffer)-1];
        while(*p == ' ' && p != &buffer[1]) p--;
        *++p = '\0';
        q = out;
        for(p = buffer; *p != '\0'; p++) {
            if (collect != 0) {
                if (*p == '\\' && p[1] == collect) {
                    collect = 0;
                } else {
                    *q++ = *p;
                    continue;
                }
            }
            switch(*p) {
            case '(':
            case ')':
                       *q++ = '\\';
                       *q++ = *p;
                       pos++;
                       break;
            case '\t':
                       i = (pos | 07) + 1;
                       while(pos != i) {
                           pos++;
                           *q++ = ' ';
                       }
                       break;
            case '<':
                       i = 0;
                       *q++ = '\0';
                       if (out[0] != '\0') {
                           strm->appendStr(out);
                           strm->appendCmd("Tj ");
                           q = out;
                       }
                       while(*++p != '>') 
                           i = (i << 3) + (*p - '0');
                       sprintf(out, "/FS 10 Tf (\\%03o) Tj /FF 10 Tf", i);
                       strm->appendCmd(out);
                       res.addFont2(font2);
                       break;
            case '\\':
                       switch(*++p) {
                       case 'A':   // Imbed file
                            *q++ = '\0';
                            if (out[0] != '\0') {
                                strm->appendStr(out);
                                strm->appendCmd("Tj ");
                                q = out;
                            }
                            collect = 'a';
                            break;

                       case 'a':
                            *q++ = '\0';
                            q = out;
                            if (out[0] != '\0') {
                                annots.ref(out, 10 + (pos * 6), 740 - line);
                                if (p[1] != '\0') {
                                    strcpy(out, "    ");
                                    i = strlen(out);
                                    q = &out[i];
                                    pos += i;
                                }
                            }
                            break;
                            
                       case 'U':   // Start underline
                            uline = new unline;
                            uline->next = ulst;
                            uline->sx = 10 + (pos * 6);
                            uline->sy = 750 - line;
                            break;

                       case 'u':   // Finish underline
                            if (uline != 0) {
                                uline->ex = 10 + (pos * 6);
                                uline->ey = 750 - line;
                                ulst = uline;
                                uline = 0;
                            }
                            break;

                       case 'l':   // Last line.
                            *q++ = '\0';
                            if (out[0] != '\0') {
                                strm->appendStr(out);
                                strm->appendCmd("Tj ");
                            }
                            q = out;
                            pos = 0;
                            if (line < (58 * 12)) {
                                sprintf(out, "%d TL T* %d TL ", (58*12) - line, spacing);
                                line = 58 * 12;
                                strm->appendCmd(out);
                            }
                            break;

                       case 'H':   // Half space.
                            *q++ = '\0';
                            if (out[0] != '\0') {
                                strm->appendStr(out);
                                strm->appendCmd("Tj ");
                            }
                            q = out;
                            pos = 0;
                            strm->appendCmd("6 TL ");
                            spacing = 6;
                            break;

                       case 'h':   // Line + half space.
                            *q++ = '\0';
                            if (out[0] != '\0') {
                                strm->appendStr(out);
                                strm->appendCmd("Tj ");
                            }
                            q = out;
                            pos = 0;
                            strm->appendCmd("18 TL ");
                            spacing = 18;
                            break;

                       case 'D':   // Double space.
                            *q++ = '\0';
                            if (out[0] != '\0') {
                                strm->appendStr(out);
                                strm->appendCmd("Tj ");
                                q = out;
                            }
                            pos = 0;
                            strm->appendCmd("24 TL ");
                            spacing = 24;
                            break;

                       case 'N':   // Normal space.
                            *q++ = '\0';
                            if (out[0] != '\0') {
                                strm->appendStr(out);
                                strm->appendCmd("Tj ");
                            }
                            q = out;
                            pos = 0;
                            strm->appendCmd("12 TL ");
                            spacing = 12;
                            break;

                       case 'S':   // Superscript 
                            *q++ = '\0';
                            if (out[0] != '\0') {
                                strm->appendStr(out);
                                strm->appendCmd("Tj ");
                                q = out;
                            }
                            if (offset == 0) {
                                strm->appendCmd("5 Ts");
                                offset = 5;
                            } else if (offset == -5) {
                                strm->appendCmd("0 Ts");
                                offset = 0;
                            }
                            break;
                       case 's':   // Subscript
                            *q++ = '\0';
                            if (out[0] != '\0') {
                                strm->appendStr(out);
                                strm->appendCmd("Tj ");
                                q = out;
                            }
                            if (offset == 0) {
                                strm->appendCmd("-5 Ts");
                                offset = -5;
                            } else if (offset == 5) {
                                strm->appendCmd("0 Ts");
                                offset = 0;
                            }
                            break;
                       case 'I':   // Image
                            *q++ = '\0';
                            if (out[0] != '\0') {
                                strm->appendStr(out);
                                strm->appendCmd("Tj ");
                                q = out;
                            }
                            collect = 'i';
                            break;
                       case 'i':   // Output image.
                            *q++ = '\0';
                            q = out;
                            if (out[0] != '\0') {
                                Image *img = new Image;
                                Stream  *is;
                                int        h, w;
                                if (!img->open((unsigned char *)out)) {
                                   delete img;
                                   break;
                                }
                                is = newStream();
                                res.addImage(img->save(is));
                                w = img->d_width();
                                h = img->d_height();
                                image = new unline;
                                image->next = ilst;
                                image->sx = 10 + (pos * 6);
                                image->sy = 750 - line - h;
                                image->ex = h;
                                image->ey = w;
                                image->num = is->obj->number;
                                line += h;
                                sprintf(out, "%d TL T* %d TL ", h, spacing);
                                strm->appendCmd(out);
                                if (p[1] != '\0') {
                                    sprintf(out, "[( ) %d ] TJ ", w/6);
                                    strm->appendCmd(out);
                                }
                                delete is;
                                delete img;
                                ilst = image;
                                image = 0;
                            }
                            break;
                       case '\'':  
                       case '>':   
                       case '<':   
                           *q++ = *p;
                           pos++;
                       }
                       break;
            case '\f':
                       *q++ = '\0';
                       if (&out[0] != '\0') {
                           strm->appendStr(out);
                           strm->appendCmd("Tj\n");
                           q = out;
                       }
                       strm->appendCmd("ET\n");
                       if (ulst) {
                           strm->appendCmd("0 g q 1 0 0 1 0 0 cm\n");
                           uline = ulst; 
                           while(uline != 0) {
                               struct unline  *u = uline->next;
                               strm->appendPoint(uline->sx, uline->sy);
                               strm->appendCmd(" m ");
                               strm->appendPoint(uline->ex, uline->ey);
                               strm->appendCmd(" l S\n");
                               delete uline;
                               uline = u;
                           }
                           ulst = 0;
                           uline = 0;
                           strm->appendCmd("Q\n");
                       }
                       if (ilst) {
                           strm->appendCmd("0 g\n");
                           image = ilst; 
                           while(image != 0) {
                               struct unline  *u = image->next;
                               sprintf(out, "q %d 0 0 %d %d %d cm /Im%d Do Q\n",
                                     image->ey, image->ex, image->sx, image->sy,
                                     image->num);
                               strm->appendCmd(out);
                               delete image;
                               image = u;
                           }
                           ilst = 0;
                           image = 0;
                       }
                       strm->close();
                       delete strm;
                       annots.put(this);
                       page->resource(res_cache.find(&res, this));
                       page->open();
                       page->close();
                       res.clear();
                       res.addFont1(font1);
                       strm = newStream();
                       page = newPage(0);
                       page->content(strm->obj);
                       strm->appendCmd("BT\n/FF 10 Tf 12 TL 1 0 0 1 10 752 Tm");
                       spacing = 12;
                       pos = 0;
                       line = 0;
                       break;
            default:
                       *q++ = *p;
                       pos++;
            }
        }
        *q++ = '\0';
        if (out[0] != '\0') {
            strm->appendStr(out);
            strm->appendCmd("Tj ");
        }
        if (offset != 0)
            strm->appendCmd("0 Ts ");
    }
    strm->appendCmd("ET\n");
    if (ulst) {
        strm->appendCmd("0 g q 1 0 0 1 0 0 cm\n");
        uline = ulst; 
        while(uline != 0) {
            struct unline  *u = uline->next;
            strm->appendPoint(uline->sx, uline->sy);
            strm->appendCmd(" m ");
            strm->appendPoint(uline->ex, uline->ey);
            strm->appendCmd(" l S\n");
            delete uline;
            uline = u;
        }
        strm->appendCmd("Q\n");
    }
    if (ilst) {
        strm->appendCmd("0 g\n");
        image = ilst; 
        while(image != 0) {
            struct unline  *u = image->next;
            sprintf(out, "q %d 0 0 %d %d %d cm /Im%d Do Q\n",
                  image->ex, image->ey, image->sx, image->sy,
                  image->num);
            strm->appendCmd(out);
            delete image;
            image = u;
        }
        ilst = 0;
        image = 0;
    }
    strm->close();
    annots.put(this);
    page->content(strm->obj);
    page->resource(res_cache.find(&res, this));
    page->open();
    page->close();
    delete strm;
    fclose(f);
    return;
}

// Convert an image into PDF file.
void
PDFfile::convertImage(char *name, Image *img, int land)
{
    Stream          *strm;
    Obj             *img_obj;
    Page            *page;
    Resource        res;
    char            buffer[100];

    strm = newStream();
    img_obj = img->save(strm);
    delete strm;
    page = newPage(land);
    res.addImage(img_obj);
    strm = newStream();
    if (land) 
        sprintf(buffer, "q 792 0 0 612 0 0 cm /Im%d Do Q\n",
            img_obj->number);
    else
        sprintf(buffer, "q 612 0 0 792 0 0 cm /Im%d Do Q\n",
            img_obj->number);
    strm->appendCmd(buffer);
    if (name) {
        res.addFont1(font1);
        strm->appendCmd("BT\n/FF 10 Tf ");
        if (land) 
            strm->appendCmd("10 TL 1 0 0 1 10 600 Tm\n");
        else
            strm->appendCmd("12 TL 1 0 0 1 10 752 Tm\n");
        strm->appendCmd("T* (");
        strm->appendString(name);
        strm->appendCmd(") Tj\nET\n");
    }
    strm->close();
    page->resource(res_cache.find(&res, this));
    page->content(strm->obj);
    page->open();
    page->close();
    delete strm;
}


