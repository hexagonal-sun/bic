BUILT_SOURCES = parser.h lex.h
AM_YFLAGS = -d
bin_PROGRAMS = bic

noinst_LIBRARIES = libbic.a

libbic_a_SOURCES = parser.y lex.l lex.h tree.c tree.h tree.def list.h	\
list.c

bic_SOURCES = main.c
bic_LDADD = libbic.a
bic_LDFLAGS = @LIBS@

lex.h: lex.l
	$(LEX) --header-file=$@ -o /dev/null $<
