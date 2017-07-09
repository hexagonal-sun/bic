SUBDIRS = $(BUILD_CPU)
BUILT_SOURCES = cfileparser.h cfilelex.h
AM_YFLAGS = -d
bin_PROGRAMS = bic

noinst_LIBRARIES = libbic.a
libbic_a_CFLAGS = -I$(srcdir)/$(BUILD_CPU)

libbic_a_SOURCES = cfileparser.y cfilelex.l lex.h tree.c tree.h		\
tree.def list.h list.c hash_table.c hash_table.h evaluate.c		\
evaluate.h ctypes.def gc.c gc.h tree-access.h typename.c typename.c

bic_SOURCES = main.c
bic_LDADD = libbic.a $(BUILD_CPU)/libbic_$(BUILD_CPU).a

%.h: %.l
	$(LEX) --header-file=$@ -o /dev/null $<
