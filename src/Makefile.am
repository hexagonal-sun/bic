BUILT_SOURCES = parser.h lex.h
AM_YFLAGS = -d
bin_PROGRAMS = bic

noinst_LIBRARIES = libbic.a

libbic_a_SOURCES = parser.y lex.l lex.h tree.c tree.h tree.def

bic_SOURCES = main.c
bic_LDADD = libbic.a
bic_LDFLAGS = @ax_cv_lib_readline@

lex.h: lex.l
	$(LEX) --header-file=$@ -o /dev/null $<
