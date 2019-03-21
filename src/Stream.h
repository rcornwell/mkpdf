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

// Handle PDF Stream objects.
//
// Objects are compressed with zlib.

#include <stdio.h>
#include <zlib.h>
#include "Obj.h"

#ifndef _STREAM_H_
#define _STREAM_H_
class   Stream {
public:
        Obj             *obj;
        unsigned int    size;
private:
        char            *extra;
        char            *buffer;
        unsigned int    len;
        unsigned int    pos;
        int             opened;
        struct strmchnk {
             int                len;
             char               *value;
             struct strmchnk    *next;
        }    *list, *last;

        void mkbuffer() {
           buffer = new char[1024];
           len = 1024;
           pos = 0;
        }

        void add() {
            struct strmchnk     *n;

            n = new struct strmchnk;
            if (last != 0) {
                last->next = n;
            } else {
                list = n;
            }
            n->len = pos;
            n->next = 0;
            n->value = buffer;
            size += pos;
            last = n;
            mkbuffer();
        }

public:
        Stream(Obj *obj) : obj(obj), size(0), extra(0), buffer(0),
                  len(0), pos(0), opened(0), list(0), last(0)
                 { mkbuffer(); }

        ~Stream() {
                struct strmchnk *l, *p;
                delete[] buffer;
                l = list;
                while(l != NULL) {
                    p = l->next;
                    delete l;
                    l = p;
                }
        }

        void appendStr(const char *text) {
             appendData(" (", 2);
             appendData(text, strlen(text));
             appendData(") ", 2);
        }

        void appendString(const char *text) {
             int  sz = strlen(text);
             while(sz > 0) {
                while(pos < len && sz > 0) {
                   if (*text == ')' || *text == '(' || *text == '\\') {
                        buffer[pos++] = '\\';
                        if (pos == len) 
                           add();
                   }
                   buffer[pos++] = *text++;
                   sz--;
                }
                if (pos == len) {
                   add();
                }
             }
        }

        void appendData(const char *data, unsigned int sz) {
             if (sz >= len) {
                struct strmchnk         *n;
                if (pos != 0)
                    add();

                n = new struct strmchnk;
                if (last != 0) {
                    last->next = n;
                } else {
                    list = n;
                }
                n->len = sz;
                n->next = 0;
                n->value = new char[sz];
                memcpy(n->value, data, sz);
                size += sz;
                last = n;
                return;
             }
             while(sz > 0) {
                while(pos < len && sz > 0) {
                   buffer[pos++] = *data++;
                   sz--;
                }
                if (pos == len) {
                   add();
                }
             }
        }
        
        void appendCmd(const char *text) {
             appendData(text, strlen(text));
        }

        void appendPoint(int x, int y)  {
             char       buffer[10];
             char       out[20];
             char       *p, *q;

             p = buffer;
             q = out;
             *q++ = ' ';
             *p++ = '\0';
             do {
                 *p++ = (x % 10) + '0';
                 x /= 10;
             } while (x != 0);
             while(*--p != '\0') {
                *q++ = *p;
             }
             *q++ = ' ';
             *p++ = '\0';
             do {
                 *p++ = (y % 10) + '0';
                 y /= 10;
             } while (y != 0);
             while(*--p != '\0') {
                *q++ = *p;
             }
             *q++ = ' ';
             appendData(out, q - out);
        }

        void flush() {
            if (pos != 0)
                add();
        }

        void open(const char *title = NULL) {
             obj->open(title);
             opened = 1;
        }

        void close() {
            char                *p, *cbuffer;
            struct strmchnk     *s, *l;
            z_stream            strm;

            if (pos != 0)
                add();
            if (!opened)
                open();
            if (size == 0) {
                obj->close();
                return;
            }
            if (list != 0 && list->next == 0) {
                delete[] buffer;
                buffer = list->value;
                delete list;
            } else {
                if (size > len) {
                    delete[] buffer;
                    buffer = new char[size];
                }
                p = buffer;
                s = list;
                while(s != NULL) {
                    memcpy(p, s->value, s->len);
                    p += s->len;
                    l = s->next;
                    delete[] s->value;
                    delete s;
                    s = l;
                }
            }
            list = last = 0;
            cbuffer = new char[size + (size/10) + 1];
            memset(&strm, 0, sizeof(z_stream));
            strm.zalloc = Z_NULL;
            strm.zfree = Z_NULL;
            strm.next_in = (Bytef *)buffer;
            strm.avail_in = size;
            strm.total_in = 0;
            strm.next_out = (Bytef *)cbuffer;
            strm.avail_out = size + 1 + (size/10);
            strm.total_out = 0;
            strm.data_type = Z_BINARY;
            deflateInit(&strm, Z_BEST_COMPRESSION);
            if (deflate(&strm, Z_FINISH) == Z_STREAM_END &&
                       strm.total_out < size) {
               obj->put("/Filter/FlateDecode");
               obj->put("Length", (int)strm.total_out);
               obj->put(">>stream\n");
               obj->putdata(cbuffer, strm.total_out);
           } else {
                obj->put("Length", (int)size);
                obj->put(">>stream\n");
                obj->putdata(buffer, size);
           }
            obj->put("endstream\nendobj\n");
            deflateEnd(&strm);
            delete[] cbuffer;
            delete[] buffer;
            buffer = 0;
        }

        void ref(const char *title = NULL) { obj->ref(title); }

        void put(const char *str) { obj->put(str); }

        void putN(const char *str) { obj->putN(str); }

        void put(const char *str, int v) { obj->put(str, v); }

        void put(const char *str, char *v) { obj->put(str, v); }

        void put(const char *str, time_t t) { obj->put(str, t); }

};
#endif

