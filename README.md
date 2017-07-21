`bic`: A Basic Interpreter for C
===========================================

This is a project that allows developers to test out and explore
C-APIs using a read-eval print loop (REPL).

Installation:

    autoreconf -i
    ./configure --enable-debug
    make
    ./src/bic

Usage
-----

When invoking bic with no arguments the user is presented with a REPL
prompt:

    BIC>

Here you can type C expressions and `#include` various system headers
to provide access to different APIs on the system, for example:

    BIC> #include <stdio.h>
    BIC> FILE *f;
    f
    BIC> f = fopen("test.txt", "w");
    BIC> fputs("Hello, World!\n", f);
    1
    BIC> 

This will cause bic to call out to the C-library fopen and fputs
functions to create a file and write a string to it.  If you now exit
bic, you should see a file `test.txt` in the current working directory
with the string "Hello, World\n" contained within it.

Notice that after evaluating an expression bic will print the result of
evaluation.  This can be useful for testing out simple expressions:

    BIC> 2 * 8 + fileno(f);
    19

You can also use bic to dive into the guts of compound data types and
get an idea of how they're implemented.  For example, we can deference
the `FILE *` pointer `f` to see all members of it as well as their
current values:

    BIC> *f;
    {
        ._flags = -72536956
        ._IO_read_ptr = 0x559057498620 ("Hello, World!
    ")
        ._IO_read_end = 0x559057498620 ("Hello, World!
    ")
        ._IO_read_base = 0x559057498620 ("Hello, World!
    ")
        ._IO_write_base = 0x559057498620 ("Hello, World!
    ")
        ._IO_write_ptr = 0x55905749862e ("")
        ._IO_write_end = 0x559057499620 ("��IW�U")
        ._IO_buf_base = 0x559057498620 ("Hello, World!
    ")
        ._IO_buf_end = 0x559057499620 ("��IW�U")
        ._IO_save_base = (nil)
        ._IO_backup_base = (nil)
        ._IO_save_end = (nil)
        ._markers = (nil)
        ._chain = 0x7fdb786624a0
        ._fileno = 3
        ._flags2 = 0
        ._old_offset = 0
        ._cur_column = 0
        ._vtable_offset = 0
        ._shortbuf = 0x559057497043 ("")
        ._lock = 0x5590574970a0
        ._offset = -1
        .__pad1 = (nil)
        .__pad2 = 0x5590574970b0
        .__pad3 = (nil)
        .__pad4 = 0x5590574972c0
        .__pad5 = 94078428082880
        ._mode = -1
        ._unused2 = 0x559057497094 ("")
    }

You can pass bic a source file and it will evaluate it, by calling a
`main` function.

Please note: this is a work in progress and quite a few C features
haven't yet been implemented.  Enough has been done for this to be
used as a basic REPL, however.

Implementation Overview
-----------------------

At the heart of bic's implementation is the `tree` object.  These are
generic objects that can be used to represent an entire program as
well as the current evaluator state.  It is implemented in `tree.h`
and `tree.c`, each object type is defined in `tree.def` and accessor
macros are defined in `tree-access.h`.

The output of the lexer & parser is a `tree` object hierarchy which is
then passed into the evaluator (`evaluator.c`).  The evaluator will
then recursively evaluate each tree element, updating the evaluator
state, thereby executing a program.

Memory is managed by a simple mark-and-sweep garbage collector (GC)
which is implemented in `gc.c`.  This works by examining the stack and
looking for possible root objects that exist on the stack, recursing
into them and setting a marker, telling the sweep phase not to free
the objects.  Any objects that are declared statically won't exist on
the stack; the programmer can let the GC know about these objects
using the `GC_TREE_DECL` macro.

Calls to functions external to the evaluator are handled in a
platform-dependent way.  Currently IA64 is the only supported platform
and the code to handle this is in the `x86_64` folder.  This works by
taking a function call `tree` object from the evaluator with all
arguments evaluated and marshalling them into a simple linked-list.
This is then traversed in assembly to move the value into the correct
register according to the IA64 calling-convention and then branching
to the function address.

Contributing
------------

Patches are welcome!  Current outstanding work:

 * Parser unification - the repl and file parser contain a _lot_ of
   code duplication.  It would be good to unify this.  Maybe using m4?

 * Handling multiple `#include` directives.  Whenever an include CPP
   directive is parsed on the REPL, we call out to the system's CPP to
   process the directive for us.  However, any definitions set (to
   avoid multiple deviations of the same object) in the headers are
   lost.  Maybe we need to implement our own CPP?  Can we pipe input
   to the system's CPP and read the output thereby maintaining state?

 * Add a '-l' flag that calls `dlopen` allowing external libraries to
   be search when evaluating function calls.
