ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS = -I$(top_srcdir)/src ${LIBXML2_CFLAGS}

bin_PROGRAMS = mkpdf

mkpdf_SOURCES = src/mkpdf.cpp src/Annot.cpp \
	src/Image.cpp src/PDFFile.cpp src/Obj.cpp

mkpdf_LDADD = ${LIBXML2_LIBS}

