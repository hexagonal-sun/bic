BUILT_SOURCES = cfileparser.h cfilelex.h replparser.h repllex.h lang_lexer.h \
	tree.def ctypes.def tree-base.h tree-access.h
SUBDIRS = $(BUILD_CPU)
AM_YFLAGS = --warnings="no-yacc" -d
noinst_PROGRAMS = gentypes genctypes gentree genaccess gengc gendump
bin_PROGRAMS = bic

noinst_LIBRARIES = libbic.a
libbic_a_CFLAGS = -I$(srcdir)/$(BUILD_CPU)

libbic_a_SOURCES = cfileparser.y cfilelex.l cfilelex.h tree.c tree.h	\
tree.def list.h list.c hash_table.c hash_table.h evaluate.c		\
evaluate.h ctypes.def tree-access.h typename.h typename.c repllex.l	\
repllex.h replparser.y repl.c repl.h pretty-printer.c			\
pretty-printer.h lex.m4 parser-lex-funcs.m4 parser.m4 util.c util.h	\
gc.c gc.h preprocess.c preprocess.h tree-base.h gc-internal.h		\
gc-internal.c tree-dump.c tree-dump.h tree-dump-primitives.h \
inspect.c inspect.h

EXTRA_DIST = c.lang
DISTCLEANFILES = tree.def ctypes.def tree-base.h tree-access.h	\
gc-internal.c tree-dump.c

bic_SOURCES = main.c
bic_LDADD = libbic.a $(BUILD_CPU)/libbic_$(BUILD_CPU).a

lang_src = lang.cpp lang.h lang_lexer.lpp lang_lexer.h

gentypes_SOURCES =  $(lang_src) gentypes.cpp
genctypes_SOURCES =  $(lang_src) genctypes.cpp
gentree_SOURCES =  $(lang_src) gentree.cpp
genaccess_SOURCES = $(lang_src) genaccess.cpp
gengc_SOURCES = $(lang_src) gengc.cpp
gendump_SOURCES = $(lang_src) gendump.cpp

tree.def: gentypes c.lang
	$(AM_V_GEN)./$^

ctypes.def: genctypes c.lang
	$(AM_V_GEN)./$^

tree-base.h: gentree c.lang
	$(AM_V_GEN)./$^

tree-access.h: genaccess c.lang
	$(AM_V_GEN)./$^

gc-internal.c: gengc c.lang
	$(AM_V_GEN)./$^

tree-dump.c: gendump c.lang
	$(AM_V_GEN)./$^

%.h: %.l
	$(AM_V_GEN)$(LEX) --header-file=$@ -o /dev/null $<

%.h: %.lpp
	$(AM_V_GEN)$(LEX) --header-file=$@ -o /dev/null $<

repllex.l: lex.m4
	$(AM_V_GEN)m4 -DTARGET=repl $< > $@

cfilelex.l: lex.m4
	$(AM_V_GEN)m4 -DTARGET=cfile $< > $@

replparser.y: parser.m4
	$(AM_V_GEN)m4 -DTARGET=repl $< > $@

cfileparser.y: parser.m4
	$(AM_V_GEN)m4 -DTARGET=cfile $< > $@

DIST_SUBDIRS = x86_64 aarch64 armv7l
