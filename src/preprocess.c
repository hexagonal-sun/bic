#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <unistd.h>

#include "tree.h"
#include "gc.h"
#include "preprocess.h"

static tree include_dirs;
GC_STATIC_TREE(include_dirs);

FILE *run_cpp(tree include_chain, char *opts, char *line)
{
  char cpp_file_name[] = "/tmp/bic.cpp.XXXXXX.c";
  char out_file_name[] = "/tmp/bic.cpp.XXXXXX.c";
  char *command;
  char *include_dir_opts = NULL;
  FILE *out_file_stream;
  int cpp_file_fd, out_file_fd, ret;
  tree i;

  cpp_file_fd = mkstemps(cpp_file_name, 2);

  if (cpp_file_fd == -1) {
    fprintf(stderr, "Could not open temp file: %s\n", strerror(errno));
    exit (1);
  }

  out_file_fd = mkstemps(out_file_name, 2);

  if (out_file_fd == -1) {
    fprintf(stderr, "Could not open temp file: %s\n", strerror(errno));
    exit (1);
  }

  out_file_stream = fdopen(out_file_fd, "r");

  if (!out_file_stream) {
    fprintf(stderr, "Could not create stream: %s\n", strerror(errno));
    exit (1);
  }

  for_each_tree (i, include_chain) {
    write(cpp_file_fd, tCPP_INCLUDE_STR(i), strlen(tCPP_INCLUDE_STR(i)));
    write(cpp_file_fd, "\n", 1);
  }

  if (line)
    write(cpp_file_fd, line, strlen(line));

  close(cpp_file_fd);

  if (include_dirs) {
      size_t include_len = 0;

      for_each_tree(i, include_dirs)
          include_len += strlen(tSTRING_VAL(i)) + 4;

      include_dir_opts = malloc(include_len + 1);
      include_dir_opts[0] = '\0';

      for_each_tree(i, include_dirs) {
          strcat(include_dir_opts, " -I ");
          strcat(include_dir_opts, tSTRING_VAL(i));
      }
  } else {
      include_dir_opts = malloc(1);
      include_dir_opts[0] = '\0';
  }

  ret = asprintf(&command, "gcc %s %s %s > %s", opts, include_dir_opts,
          cpp_file_name, out_file_name);

  free(include_dir_opts);

  if (ret == -1) {
    fprintf(stderr, "Could not compose preprocessor command: %s\n",
         strerror(errno));
    exit (1);
  }

  ret = system(command);

  free(command);

  if (ret < 0) {
    fprintf(stderr, "Invocation of preprocessor process failed\n");
    exit (1);
  }

  if (ret > 0) {
    fprintf(stderr, "Preprocessor command exited with non-zero status\n");
    exit (1);
  }

  return out_file_stream;
}

void preprocessor_add_include_dir(char *dir)
{
    tree new_dir;

    if (!include_dirs)
        include_dirs = tree_make(CHAIN_HEAD);

    new_dir = tree_make(T_STRING);

    tSTRING_VAL(new_dir) = strdup(dir);

    tree_chain(new_dir, include_dirs);
}
