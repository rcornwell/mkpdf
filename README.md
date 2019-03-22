# MKPDF

Mkpdf is a small program I put together to convert diagnostic writups and listings
into a PDF document for easier reading. This tool also includes some simple graphics
processing with PNG files. Source and binary files can also be annotated in the file
so that only one file need to kept to have everything needed to run and use a diagnostic.
This program can also be used to clean up scanned listings to make them easier to read.

Mkpdf takes a XML configuration file as follows:

    <?xml version="1.0"?>
    <!DOCTYPE PDFFile>
    <PDFFile name="9a01a.pdf" title="9A01A">
       <section name="Writeup">
        <attachment id="deck" name="9a01a.dck" type="binary">deck</attachment>
        <attachment id="card" name="9a01a.cbn" type="binary">deck</attachment>
        <attachment id="source" name="9a01a.asm" type="ascii">source</attachment>
        <text name="9a01a.txt"/>
        <image name="9a01a-1.png"><threshold/></image>
        <image name="9a01a-2.png"><threshold/></image>
        <image name="9a01a-3.png"><threshold/></image>
       </section>
       <section name="Listing">
          <listing name="9a01a.lst" linesperpage="59"/>
       </section>
    </PDFFile>

The document must have:

    <?xml version="1.0"?>
    
    <PDFFile>
    </PDFFile>

## \<PDFFile>

PDFFile requires a name="" option, this is the name of the resulting PDF file.
Optionally a title="" option can be given, to be the title displayed.
PDFfile consists of a series of \<section> tags. Optionally it can include a
series of \<image>, \<attachment>, \<text>, \<portrat>, \<landscape>, or \<listing> items.

## \<section>

Section requires a name="" option to be listing in the documents table of contents.
Sections can include \<image>, \<attachment>, \<text>, \<portrat>, \<landscape>, or \<listing>
items.

## \<image>

Image section allows for embedded PNG files. Name="" option is required to indicate
the name of the PNG file to be included. This include:

* \<threshold>
* \<avg>
* \<contrast>
* \<unsharp>
* \<label>
* \<protrat>
* \<landscape>
* \<cw>
* \<ccw>
* \<rotate>
* \<flip>
* \<reverse>
* \<transpose>
* \<edgefill>

These tags cause various image processing functions to be run.

## \<text>

Text includes a text file with basic formating options. Name="" is required and indicates
the name of the file to include. This can include an optional data element of "portrat" or
"landscape" to set the orientation of the included text.

### text formating controls.

* \<#>   - Allows for Unicode control character to be include.
* \Aname\a   - Denotes an attached file.
* \Utext\u   - Underlines the enclosed text.
* \l      - indicates the last line of a page.
* \H      - Sets half line spacing.
* \h      - Sets line plus half line spacing.
* \D      - Sets double spacing.
* \N      - Sets normal spacing.
* \Stext\s  - Superscripts the text.
* \stext\S  - Subscripts the text.
* \Iname\i  - Embeds an image into the text.
* \<        - Inserts a <
* \>        - Inserts a >
* control-F - Skips to a new page.


## \<listing>

This tag includes a listing file into the document. Name="" is required and indicates the 
name of the file to include. Optionally linesperpage="#", the default is 55. This can
include an optional data element of "portrat" or "landscape" to set the orientation of the
included listing.

## \<attachment>

This tag can be added to any section and allows for the embedding of a file. Name="" is 
required and indicates the name of the file to attach. Type="binary" or type="ascii" 
indicates the type of the file to include. This file is compressed automatically by the embedding.

## \<portrat>

This sets the page layout to portrat mode for all tags below.

## \<landscape>

This set the page layout to landscape mode for all tags below.


