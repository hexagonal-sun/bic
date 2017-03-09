BUILT_SOURCES = parser.h lex.h
AM_YFLAGS = -d
bin_PROGRAMS = bic

noinst_LIBRARIES = libbic.a

libbic_a_SOURCES = parser.y lex.l lex.h tree.c tree.h tree.def list.h	\
list.c hash_table.c hash_table.h identifier.c identifier.h

bic_SOURCES = main.c
bic_LDADD = libbic.a

lex.h: lex.l
	$(LEX) --header-file=$@ -o /dev/null $<
