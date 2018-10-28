SUBDIRS = $(BUILD_CPU)
BUILT_SOURCES = cfileparser.h cfilelex.h replparser.h repllex.h
AM_YFLAGS = -d
bin_PROGRAMS = bic

noinst_LIBRARIES = libbic.a
libbic_a_CFLAGS = -I$(srcdir)/$(BUILD_CPU)

libbic_a_SOURCES = cfileparser.y cfilelex.l cfilelex.h tree.c tree.h	\
tree.def list.h list.c hash_table.c hash_table.h evaluate.c		\
evaluate.h ctypes.def tree-access.h typename.h typename.c repllex.l	\
repllex.h replparser.y repl.c repl.h pretty-printer.c			\
pretty-printer.h lex.m4 parser-lex-funcs.m4 parser.m4 util.c util.h	\
gc.c gc.h preprocess.c preprocess.h

bic_SOURCES = main.c
bic_LDADD = libbic.a $(BUILD_CPU)/libbic_$(BUILD_CPU).a

%.h: %.l
	$(LEX) --header-file=$@ -o /dev/null $<

repllex.l: lex.m4
	m4 -DTARGET=repl $< > $@

cfilelex.l: lex.m4
	m4 -DTARGET=cfile $< > $@

replparser.y: parser.m4
	m4 -DTARGET=repl $< > $@

cfileparser.y: parser.m4
	m4 -DTARGET=cfile $< > $@

DIST_SUBDIRS = x86_64 aarch64
