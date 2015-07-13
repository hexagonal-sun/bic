BUILT_SOURCES = parser.h
AM_YFLAGS = -d
bin_PROGRAMS = bic

noinst_LIBRARIES = libbic.a

libbic_a_SOURCES = parser.y lex.l

bic_SOURCES = main.c
bic_LDADD = libbic.a
