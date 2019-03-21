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
// to make them easier to read. This is the main driver program.
//
 
#include <stdio.h>
#include <string.h>
#include <png.h>
#include <sys/stat.h>
#include <unistd.h>

#include "Obj.h"
#include "Stream.h"
#include "Page.h"
#include "PDFFile.h"
#include "Image.h"
#include "Annot.h"
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xinclude.h>
#include <libxml/xmlIO.h>   

int         verbose = 0;            // Verbose flag
int         landscape = 0;          // Are we portrat or landscape mode
char        *in_section = 0;        // Inside a section, no section allowed.
const char  *in_node = 0;           // Inside file node.


void parseDoc(char *docname);
void parseAttach(PDFfile *file, xmlDoc *doc, xmlNodePtr cur);
void parseSection(PDFfile *file, xmlDoc *doc, xmlNodePtr cur);
void parseImage(PDFfile *file, xmlDoc *doc, xmlNodePtr cur);
void parseText(PDFfile *file, xmlDoc *doc, xmlNodePtr cur);
void parsePortrat(PDFfile *file, xmlDoc *doc, xmlNodePtr cur);
void parseLandscape(PDFfile *file, xmlDoc *doc, xmlNodePtr cur);
void parseListing(PDFfile *file, xmlDoc *doc, xmlNodePtr cur);
void parseFile(PDFfile *file, xmlDoc *doc, xmlNodePtr cur);

struct XMLnode {
    xmlChar     *name;
    void        (*parse)(PDFfile *file, xmlDoc *doc, xmlNodePtr cur);
};

// Define sections in the file.
// Section name and procedure to process that section.
struct XMLnode file_table[] = {
         {(xmlChar *)"section", parseSection}, 
         {(xmlChar *)"image",   parseImage}, 
         {(xmlChar *)"attachment", parseAttach}, 
         {(xmlChar *)"text",     parseText}, 
         {(xmlChar *)"portrat",  parsePortrat}, 
         {(xmlChar *)"landscape", parseLandscape}, 
         {(xmlChar *)"listing",   parseListing}, 
         {(xmlChar *)NULL,        NULL},
};

// Define XML DTD defintion for parser 
char PDFfileDTD[] = 
      "<?xml version=\"1.0\" encoding=\"ascii\" ?>"
      "<!ELEMENT PDFFile (#PCDATA|section|image|attachment|text|portrat"
           "|landscape|listing)*>"
      "<!ATTLIST PDFFile name CDATA #REQUIRED title CDATA #IMPLIED>"
      "<!ELEMENT section (#PCDATA|image|attachment|text|portrat|landscape"
                "|listing)*>"
      "<!ATTLIST section name CDATA #REQUIRED>"
      "<!ELEMENT attachment (#PCDATA)*>"
      "<!ATTLIST attachment type (ascii|binary) \"ascii\""
               " name CDATA #REQUIRED id ID #REQUIRED>"
      "<!ELEMENT text (#PCDATA|landscape|portrat)*>"
      "<!ATTLIST text name CDATA #REQUIRED>"
      "<!ELEMENT portrat (#PCDATA|section|image|attachment|text|landscape|portrat)*>"
      "<!ELEMENT landscape (#PCDATA|section|image|attachment|text|landscape|portrat)*>"
      "<!ELEMENT listing (#PCDATA|landscape|portrat)*>"
      "<!ATTLIST listing name CDATA #REQUIRED"
                 " linesperpage CDATA \"55\">"
      "<!ELEMENT image (#PCDATA|threshold|avg|contrast|unsharp|label|portrat|landscape|"
          "cw|ccw|rotate|flip|reverse|transpose|edgefill)*>"
      "<!ATTLIST image name CDATA #REQUIRED>"
      "<!ELEMENT contrast (#PCDATA)*>"
      "<!ATTLIST contrast angle CDATA #IMPLIED bright CDATA #IMPLIED>"
      "<!ELEMENT unsharp (#PCDATA)*>"
      "<!ATTLIST unsharp thresh CDATA #IMPLIED"
                " radius CDATA #IMPLIED amount CDATA #IMPLIED>"
      "<!ATTLIST threshold value CDATA #IMPLIED>"
      "<!ELEMENT label (#PCDATA)*>"
      "<!ELEMENT threshold (#PCDATA)*>"
      "<!ELEMENT avg (#PCDATA)*>"
      "<!ATTLIST avg offset CDATA #IMPLIED>"
      "<!ELEMENT edgefill (#PCDATA)*>"
      "<!ELEMENT cw (#PCDATA)*>"
      "<!ELEMENT ccw (#PCDATA)*>"
      "<!ELEMENT rotate (#PCDATA)*>"
      "<!ELEMENT flip (#PCDATA)*>"
      "<!ELEMENT reverse (#PCDATA)*>"
      "<!ELEMENT transpose (#PCDATA)*>";


// Main program.
//
// Accepts a option of -v to display progress. And the name of a XML control file.
//
int
main(int argc, char *argv[])
{
    char    *p;

    while(--argc > 0) {
        p = *++argv;
        if (*p == '-' && p[1] == 'v') {
            verbose = 1;
        } else {
            parseDoc(p);
        }
    }
}

//
// Parse the control document.
//
void
parseDoc(char *docname)
{
    PDFfile *file;
    xmlDocPtr               doc;
    xmlNodePtr              cur;
    xmlChar                 *name;
    xmlChar                 *title;
    xmlParserInputBufferPtr dtd_txt;
    xmlDtdPtr               dtd;
    xmlValidCtxtPtr         v_ctxt;

    // Open document with libXML.
    dtd_txt = xmlParserInputBufferCreateMem(PDFfileDTD, sizeof(PDFfileDTD),
                    XML_CHAR_ENCODING_NONE);

    // Load DTD for document.
    dtd = xmlIOParseDTD(NULL, dtd_txt, XML_CHAR_ENCODING_ASCII);
    
    if (dtd == NULL) {
        fprintf(stderr, "Could not load DTD\n");
        return;
    }

    // Apply DTD to document.
    doc = xmlParseFile(docname);

    if (doc == NULL ) {
        fprintf(stderr,"Document not parsed successfully. \n");
        xmlFreeDtd(dtd);
        return;
    }

    // Validate document against DTD
    v_ctxt = xmlNewValidCtxt();

    if (xmlValidateDtd(v_ctxt, doc, dtd) == 0) {
        fprintf(stderr, "Document not valid for program.\n");
        xmlFreeDtd(dtd);
        xmlFreeDoc(doc);
        xmlFreeValidCtxt(v_ctxt);
        return;
    }

    if (xmlValidateDtdFinal(v_ctxt, doc) == 0) {
        fprintf(stderr, "Document not valid for program.\n");
        xmlFreeDtd(dtd);
        xmlFreeDoc(doc);
        xmlFreeValidCtxt(v_ctxt);
        return;
    }

    // Walk down the nodes in the control document.
    cur = xmlDocGetRootElement(doc);

    if (cur == NULL) {
        fprintf(stderr,"empty document\n");
        xmlFreeDoc(doc);
        return;
    }

    // Make sure correct type of document.
    if (xmlStrcmp(cur->name, (const xmlChar *) "PDFFile")) {
        fprintf(stderr,"document of the wrong type, root node != PDFFile");
        xmlFreeDoc(doc);
        return;
    }

    // Document must have a name attached.
    name = xmlGetProp(cur, (const xmlChar *)"name");
    if (name == NULL) {
        fprintf(stderr, "No document name specified\n");
        xmlFreeDoc(doc);
        return;
    }

    // Create a new PDF file based on the name.
    file = new PDFfile();
    if (file->open((char *)name)) {
        fprintf(stderr, "Unable to create PDF file %s\n", name);
        xmlFreeDoc(doc);
        delete file;
        return;
    }

    if(verbose)
       fprintf(stderr, "Writing %s\n", name);

    // Find title element.
    title = xmlGetProp(cur, (const xmlChar *)"title");
    if (title == NULL) 
        file->title((char *)name);
    else {
        if (verbose)
            fprintf(stderr, "Title %s\n", title);
        file->title((char *)title);
        xmlFree(title);
    }

    // Create common objects in PDF file.   
    file->font1 = file->newObj();
    file->font1->open("Font");
    file->font1->put("/Subtype/Type1/BaseFont/Courier");
    file->font1->close();

    file->font2 = file->newObj();
    file->font2->open("Font");
    file->font2->put("/Subtype/Type1/BaseFont/Symbol");
    file->font2->close();

    // Parse rest of document.
    parseFile(file, doc, cur);

    // All done, clean up house.
    if (verbose)
        fprintf(stderr, "Closing\n");
    file->close();
    delete file;
    if (verbose)
        fprintf(stderr, "Done\n");
    xmlFreeDtd(dtd);
    xmlFree(name);
    xmlFreeValidCtxt(v_ctxt);
    xmlFreeDoc(doc);
    return;
}                                                          

//
// Parse individual elements in the file.
// Find element in element table and call correct routine to parse it.
void
parseFile(PDFfile *file, xmlDoc *doc, xmlNodePtr cur)
{
    XMLnode     *np;

    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
       if (cur->type == XML_ELEMENT_NODE) {
           for(np = file_table; np->name != NULL; np++) {
                if (xmlStrcmp(cur->name, np->name) == 0) {
                    np->parse(file, doc, cur);
                    break;
                }
           }
           if (np->name == NULL) 
               fprintf(stderr, "Unknown type %s\n", cur->name);
        }
        cur = cur->next;
    }
}
        
//
// Parse a attachment item. This will embed the file within the PDF.
void
parseAttach(PDFfile *file, xmlDoc *doc, xmlNodePtr cur)
{
    xmlChar     *name;
    xmlChar     *id;
    xmlChar     *type;
    Annot       *a;
    int         mode = 1;
    
    name = xmlGetProp(cur, (const xmlChar *)"name");
    if (name == NULL) {
        fprintf(stderr, "Attachment tag missing name attribute\n");
        return;
    }
    a = file->newAnnot();
    id = xmlGetProp(cur, (const xmlChar *)"id");
    type = xmlGetProp(cur, (const xmlChar *)"type");
    if (xmlStrcmp(type, (const xmlChar *)"ascii") == 0) {
        mode = 0;
    } else if (xmlStrcmp(type, (const xmlChar *)"binary") == 0) {
        mode = 1;
    } else {
        fprintf(stderr, "attachement Unknown type %s\n", type);
    }
    type = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
    a->load(file, (char *)name, (type)?((char *)type):"file", mode);
    xmlFree(name);
    if (type)
        xmlFree(type);
    if (id) {
        a->setid((char *)id);
        xmlFree(id);
    }
}

//
// Parse a text file and put the result into the document.
void
parseText(PDFfile *file, xmlDoc *doc, xmlNodePtr cur)
{
     xmlChar    *name;
     int        l = landscape;

     if (in_node) {
         fprintf(stderr, "<text> Not allowed in <%s>\n", in_node);
         return;
     }
     in_node = "text";
     name = xmlGetProp(cur, (const xmlChar *)"name");
     if (name == NULL) {
         fprintf(stderr, "Text tag missing name attribute\n");
         return;
     }
     parseFile(file, doc, cur);
     file->convertText((char *)name);
     in_node = 0;
     landscape = l;     // Restore landscape 
     xmlFree(name);
}

//
// Parse a landscape image.
void
parseLandscape(PDFfile *file, xmlDoc *doc, xmlNodePtr cur)
{
    landscape = 1;
    // Check to make sure no nodes below.
    parseFile(file, doc, cur);
}

//
// Parse a portrat image.
void
parsePortrat(PDFfile *file, xmlDoc *doc, xmlNodePtr cur)
{
    landscape = 0;
    parseFile(file, doc, cur);
}

//
// Parse a listing file.
void
parseListing(PDFfile *file, xmlDoc *doc, xmlNodePtr cur)
{
    xmlChar     *name;
    xmlChar     *temp;
    xmlChar     *p;
    int         lpp = 55;
    int         l = landscape;

     if (in_node) {
         fprintf(stderr, "<text> Not allowed in <%s>\n", in_node);
         return;
    }
    in_node = "listing";

    // Name tag is required.
    name = xmlGetProp(cur, (const xmlChar *)"name");
    if (name == NULL) {
        fprintf(stderr, "Text tag missing name attribute\n");
        return;
    }
    // Optional linesperpage
    temp = xmlGetProp(cur, (const xmlChar *)"linesperpage");
    if (temp != NULL) {
        int     v = 0;
        for(p = temp; *p != '\0'; p++) {
            if (*p >= '0' && *p <= '9') {
                v = (v*10) + (*p - '0');
            } else {
                fprintf(stderr, "Invalid linesperpage, not numeric (%s)\n", temp);
                v = lpp;
                break;
            }
        }
        xmlFree(temp);
        lpp = v;
    }
    // Process children, only portrat or landscape allowed 
    parseFile(file, doc, cur);
    file->convertFile((char *)name, lpp, l);
    landscape = l;
    in_node = 0;
    xmlFree(name);
}

//
// Do contrast enhancement on image.
void
parseContrast(PDFfile *file, xmlDoc *doc, xmlNodePtr cur, Image *img)
{
    int                 angle = 45;
    int                 bright = 0;
    xmlChar             *num;

    num = xmlGetProp(cur, (const xmlChar *)"angle");
    if (num != 0) {
        xmlChar *p = num;
        int     v = 0;
        int     sign = 1;
        if (*p == '-') {
            sign = -1;
            p++;
        }
        for(; *p != '\0'; p++) {
             if (*p >= '0' && *p <= '9') {
                 v = (v*10) + (*p - '0');
             } else {
                 fprintf(stderr, "Invalid angle, not numeric (%s)\n", num);
                 v = angle;
                 break;
             }
        }
        xmlFree(num);
        angle = sign * v;
    }
    num = xmlGetProp(cur, (const xmlChar *)"bright");
    if (num != 0) {
        xmlChar *p = num;
        int     v = 0;
        int     sign = 1;
        if (*p == '-') {
            sign = -1;
            p++;
        }
        for(; *p != '\0'; p++) {
             if (*p >= '0' && *p <= '9') {
                 v = (v*10) + (*p - '0');
             } else {
                 fprintf(stderr, "Invalid bright, not numeric (%s)\n", num);
                 v = bright;
                 break;
             }
        }
        xmlFree(num);
        bright = sign * v;
    }
    img->contrast(angle, bright);
}

//
// Unsharpen an image.
void
parseUnsharp(PDFfile *file, xmlDoc *doc, xmlNodePtr cur, Image *img)
{
    int                 radius = 50;
    int                 thresh = 20;
    int                 amount = 50;
    xmlChar             *num;

    num = xmlGetProp(cur, (const xmlChar *)"radius");
    if (num != 0) {
        xmlChar *p = num;
        int     v = 0;
        int     sign = 1;
        p = num;
        if (*p == '-') {
            sign = -1;
            p++;
        }
        for (; *p != '\0'; p++) {
            if (*p >= '0' && *p <= '9') {
                v = (v*10) + (*p - '0');
            } else {
                fprintf(stderr, "Invalid radius, not numeric (%s)\n", num);
                v = radius;
                break;
            }
        }
        xmlFree(num);
        radius = sign * v;
    }
    num = xmlGetProp(cur, (const xmlChar *)"thresh");
    if (num != 0) {
        xmlChar *p = num;
        int     v = 0;
        int     sign = 1;
        p = num;
        if (*p == '-') {
            sign = -1;
            p++;
        }
        for (; *p != '\0'; p++) {
             if (*p >= '0' && *p <= '9') {
                 v = (v*10) + (*p - '0');
             } else {
                 fprintf(stderr, "Invalid thresh, not numeric (%s)\n", num);
                 v = thresh;
                 break;
             }
        }
        xmlFree(num);
        thresh = sign * v;
    }
    num = xmlGetProp(cur, (const xmlChar *)"amount");
    if (num != 0) {
        xmlChar *p = num;
        int     v = 0;
        int     sign = 1;
        p = num;
        if (*p == '-') {
            sign = -1;
            p++;
        }
        for (; *p != '\0'; p++) {
             if (*p >= '0' && *p <= '9') {
                 v = (v*10) + (*p - '0');
             } else {
                 fprintf(stderr, "Invalid amount, not numeric (%s)\n", num);
                 v = amount;
                 break;
             }
        }
        xmlFree(num);
        amount = sign * v;
    }
    img->unsharp(amount, radius, thresh);
}

//
// Process an embedded image.
void
parseImage(PDFfile *file, xmlDoc *doc, xmlNodePtr cur)
{
    xmlChar             *name;
    int                 land = 0;
    int                 label = 0;
    Image               *img;

    name = xmlGetProp(cur, (const xmlChar *)"name");
    if (name == NULL) {
        fprintf(stderr, "Text tag missing name attribute\n");
        return;
    }
    img = new Image();
    if (!img->open(name)) {
        fprintf(stderr, "Could not open image %s\n", name);
        xmlFree(name);
        delete img;
        return;
    }
    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
        if (cur->type == XML_ELEMENT_NODE) {
            if (xmlStrcmp(cur->name, (const xmlChar *)"threshold") == 0) {
                int thresh = 127;
                xmlChar *num;
                num = xmlGetProp(cur, (const xmlChar *)"value");
                if (num != NULL) {
                    thresh = atoi((char *)num);
                    xmlFree(num);
                }
                img->thresh(thresh);
            } else if (xmlStrcmp(cur->name, (const xmlChar *)"avg") == 0) {
                int thresh = 0;
                xmlChar *num;
                num = xmlGetProp(cur, (const xmlChar *)"offset");
                if (num != NULL) {
                    thresh = atoi((char *)num);
                    xmlFree(num);
                }
                img->avg(thresh);
            } else if (xmlStrcmp(cur->name, (const xmlChar *)"contrast") == 0) {
                parseContrast(file, doc, cur, img);
            } else if (xmlStrcmp(cur->name, (const xmlChar *)"unsharp") == 0) {
                parseUnsharp(file, doc, cur, img);
            } 
            else if (xmlStrcmp(cur->name, (const xmlChar *)"label") == 0) 
                label = 1;
            else if (xmlStrcmp(cur->name, (const xmlChar *)"portrat") == 0) 
                land = 0;
            else if (xmlStrcmp(cur->name, (const xmlChar *)"landscape") == 0) 
                land = 1;
            else if (xmlStrcmp(cur->name, (const xmlChar *)"cw") == 0) 
                img->rotater90();
            else if (xmlStrcmp(cur->name, (const xmlChar *)"ccw") == 0) 
                img->rotatel90();
            else if (xmlStrcmp(cur->name, (const xmlChar *)"rotate") == 0) 
                img->rotate180();
            else if (xmlStrcmp(cur->name, (const xmlChar *)"flip") == 0) 
                img->flip();
            else if (xmlStrcmp(cur->name, (const xmlChar *)"reverse") == 0) 
                img->reverse();
            else if (xmlStrcmp(cur->name, (const xmlChar *)"transpose") == 0) 
                img->transpose();
            else if (xmlStrcmp(cur->name, (const xmlChar *)"edgefill") == 0) 
                img->boardFill();
            else 
                fprintf(stderr, "tag listing: Unknown type %s\n", cur->name);
        }
        cur = cur->next;
    }
    file->convertImage((label)?(char *)name:0, img, land);
    delete img;
    xmlFree(name);
}

//
// Parse and add a new section to PDF document.
void
parseSection(PDFfile *file, xmlDoc *doc, xmlNodePtr cur)
{
    xmlChar     *name;
    int         l = landscape;
    
    if (in_node) {
        fprintf(stderr, "Section not allowed inside <%s>\n", in_node);
        return;
    }
    if (in_section) {
        fprintf(stderr, "Section not allowed inside section name=%s\n",
                 in_section);
        return;
    }
    name = xmlGetProp(cur, (const xmlChar *)"name");
    if (name == NULL) {
        fprintf(stderr, "Section missing name, ignored\n");
        return;
    }
    file->addSection((char *)name);
    in_section= (char*)name;
    parseFile(file, doc, cur);
    in_section = 0;
    landscape = l;
    xmlFree(name);
}

