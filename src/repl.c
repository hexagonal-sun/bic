#define _GNU_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <pwd.h>

#include "config.h"
#include "cscript.h"
#include "tree.h"
#include "typename.h"
#include "evaluate.h"
#include "gc.h"
#include "replparser.h"
#include "repllex.h"
#include "util.h"
#include "pretty-printer.h"
#include "preprocess.h"

#ifdef HAVE_LIBREADLINE
#  if defined(HAVE_READLINE_READLINE_H)
#    include <readline/readline.h>
#  elif defined(HAVE_READLINE_H)
#    include <readline.h>
#  else /* !defined(HAVE_READLINE_H) */
extern char *readline ();
#  endif /* !defined(HAVE_READLINE_H) */
char *cmdline = NULL;
#else /* !defined(HAVE_READLINE_READLINE_H) */
#error "No readline found"
#endif /* HAVE_LIBREADLINE */

#ifdef HAVE_READLINE_HISTORY
#  if defined(HAVE_READLINE_HISTORY_H)
#    include <readline/history.h>
#  else
extern void add_history (const char *);
#  endif /* HAVE_READLINE_HISTORY_H */
#endif /* HAVE_READLINE_HISTORY */

tree repl_parse_head;
GC_STATIC_TREE(repl_parse_head);

void replerror(const char *str)
{
    fprintf(stderr, "Parser Error: %s.\n", str);
}

static int is_compound_access(const char *text)
{
    size_t i, length = strlen(text);

    if (length < 2)
        return 0;

    for (i = length - 1; i > 0; i--) {

        if (text[i] == '.')
            return 1;

        if (strstr(&text[i], "->"))
            return 1;
    }

    return 0;
}

struct completion_comp_access
{
    char *obj_expr;
    char *access_operator;
    char *member_prefix;
    int needs_deref;
};

static int split_compound_access_text(const char *text,
                                      struct completion_comp_access *comp_access)
{
    size_t i, text_len = strlen(text);

    memset(comp_access, 0, sizeof(*comp_access));

    for (i = text_len; i > 0; i--) {
        if (text[i] == '.') {
            comp_access->obj_expr = strdup(text);
            comp_access->obj_expr[i] = '\0';

            comp_access->access_operator = strdup(".");
            comp_access->needs_deref = 0;

            comp_access->member_prefix = strdup(&text[i + 1]);
            break;
        }

        if (strstr(&text[i], "->")) {
            comp_access->obj_expr = strdup(text);
            comp_access->obj_expr[i] = '\0';

            comp_access->access_operator = strdup("->");
            comp_access->needs_deref = 1;

            comp_access->member_prefix = strdup(&text[i + 2]);
        }
    }

    if (!comp_access->obj_expr)
        return 0;

    return 1;
}

static tree get_compound_completion_object(struct completion_comp_access comp_access)
{
    char *comp_obj_expr = strdup(comp_access.obj_expr);

    if (comp_access.needs_deref) {
        comp_obj_expr = concat_strings("*(", comp_obj_expr);
        comp_obj_expr = concat_strings(comp_obj_expr, ")");
    }

    /* Append a ';' to the `obj_expr' to make it a valid C
     * expression. */
    comp_obj_expr = concat_strings(comp_obj_expr, ";");

    YY_BUFFER_STATE lex_buffer = repl_scan_string(comp_obj_expr);
    inhibit_gc();
    int parse_result = replparse();
    enable_gc();
    repl_delete_buffer(lex_buffer);

    if (parse_result)
        return NULL;

    return evaluate_expr(repl_parse_head);
}

void match_identifiers_for_idmap(tree chain, tree idmap,
                                 const char *prefix)
{
    tree i;

    for_each_tree(i, idmap) {
        tree id = tEMAP_LEFT(i);
        if (strncmp(tID_STR(id), prefix, strlen(prefix)) == 0)
            tree_chain(id, chain);
    }
}

static char **create_matches_array(const char *original_text, tree chain)
{
    tree i;
    char **ret;
    size_t match_count = 0;

    for_each_tree(i, chain)
        match_count++;

    if (match_count == 0)
        return NULL;

    /* For a single match, replace `prefix' with the completed
     * text. */
    if (match_count == 1) {
        ret = calloc(2, sizeof(*ret));

        for_each_tree(i, chain)
            ret[0] = strdup(tID_STR(i));

        ret[1] = NULL;

        return ret;
    }

    /* If there are multiple matches, return `original_text' as the
     * first element.  Readline interprets the first element in the
     * array as the string to replace whatever is being completed
     * with. */

    ret = calloc(match_count + 2, sizeof(*ret));

    match_count = 0;

    ret[match_count] = strdup(original_text);

    for_each_tree(i, chain) {
        ret[match_count + 1] = strdup(tID_STR(i));
        match_count++;
    }

    ret[match_count + 1] = NULL;

    return ret;
}

static char **do_compound_completion(struct completion_comp_access comp_access,
                                     const char *original_text,
                                     tree live_compound)
{
    tree i, matches = tree_make(CHAIN_HEAD),
        matching_exprs = tree_make(CHAIN_HEAD);

    char *access_string = concat_strings(comp_access.obj_expr,
                                         comp_access.access_operator);

    match_identifiers_for_idmap(matches, tLV_COMP_MEMBERS(live_compound),
                                comp_access.member_prefix);

    /* For each match, we need to build the full string that would
     * make the match.  For example, replace `foomember' with
     * `bar.baz->foomember'. */
    for_each_tree(i, matches) {
        tree expr = get_identifier(concat_strings(access_string, tID_STR(i)));
        tree_chain(expr, matching_exprs);
    }

    return create_matches_array(original_text, matching_exprs);
}

static char **do_identifier_completion(const char *text)
{
    tree matches = find_global_identifiers(text);

    return create_matches_array(text, matches);
}

static void send_matches(char **matches, int fd)
{
    size_t i, len = 0;

    /* Count the number of matching strings. */
    while (matches && matches[len])
        len++;

    write(fd, &len, sizeof(len));

    if (!matches)
        return;

    /* Send each string. */
    for (i = 0; matches[i]; i++) {
        len = strlen(matches[i]) + 1;

        write(fd, &len, sizeof(len));
        write(fd, matches[i], len);
    }
}

static char **recieve_matches(int fd)
{
    char **ret;
    size_t n;
    int i;

    read(fd, &n, sizeof(n));

    if (!n)
        return NULL;

    ret = malloc(sizeof(*ret) * (n + 1));

    for (i = 0; i < n; i++) {
        size_t match_len;

        read(fd, &match_len, sizeof(match_len));

        ret[i] = malloc(match_len);

        read(fd, ret[i], match_len);
    }

    ret[n] = NULL;

    return ret;
}

static char **bic_completion(const char *text, int start, int end)
{
    char **matches = NULL;

    if (is_compound_access(text)) {
        tree live_compound;
        struct completion_comp_access comp_access;
        pid_t ret;
        int pfds[2];

        if (!split_compound_access_text(text, &comp_access))
            return NULL;

        if (pipe(pfds) == -1) {
            perror("Error: pipe failed");
            return NULL;
        }

        ret = fork();

        if (ret == -1) {
            perror("Error: fork failed");
            return NULL;
        }

        if (!ret) {
            close(pfds[0]);
            char **local_matches = NULL;

            live_compound = get_compound_completion_object(comp_access);

            if (is_T_LIVE_COMPOUND(live_compound))
                local_matches = do_compound_completion(comp_access,
                                                       text, live_compound);

            send_matches(local_matches, pfds[1]);

            close(pfds[1]);

            exit(0);
        } else {
            close(pfds[1]);

            matches = recieve_matches(pfds[0]);

            free(comp_access.obj_expr);
            free(comp_access.access_operator);
            free(comp_access.member_prefix);

            close(pfds[1]);
        }
    }
    else
        matches = do_identifier_completion(text);

    return matches;
}

static void setup_readline()
{
    rl_basic_word_break_characters = " ";
    rl_attempted_completion_function = bic_completion;
}

static char *run_cpp_on_line(char *line)
{
    char *ret;
    const tree include_chain = get_include_chain();
    FILE *output = run_cpp(include_chain, "-E -P", line);
    char c = 0;
    size_t pos = 0,line_len = 0;

    fseek(output, 0, SEEK_END);

    do {
        if (fseek(output, -2, SEEK_CUR) == -1) {
            perror("fseek");
            exit(1);
        }

        c = fgetc(output);
        line_len++;
    } while (c != '\n' && ftell(output) - 1);

    if (fseek(output, -1, SEEK_CUR) == -1) {
        perror("fseek");
        exit(1);
    }

    ret = calloc(line_len + 1, sizeof(*ret));

    while (pos != line_len)
        ret[pos++] = fgetc(output);

    return ret;
}

static tree repl_do_parse(char *line)
{
    int parse_result;
    tree i, ret = NULL;

    YY_BUFFER_STATE buffer = repl_scan_string(line);
    inhibit_gc();
    parse_result = replparse();
    enable_gc();
    repl_delete_buffer(buffer);

    if (parse_result)
        return NULL;

    for_each_tree(i, repl_parse_head) {
        if (ret) {
            fprintf (stderr,
                     "Error: The REPL only supports single"
                     " C statements per line.\n");
            exit (1);
        }

        ret = i;
    }

    return ret;
}

static tree repl_parse_line(char *line)
{
    tree stmt;
    char *line_after_cpp;
    /* Parsing a line happens in two passes.  We firstly parse the
     * line as given by the user to see whether we are including a
     * header file.
     *
     * If that is the case we simply pass that down to the evaluator
     * to add to the `include_chain'.
     *
     * If not, then we copy the line to the end of a small C file that
     * has all the include statments from `inline_chain' before it,
     * preprocess it and then parse that as the tree to be
     * evaluated. */

    stmt = repl_do_parse(line);

    if (!stmt)
        return NULL;

    if (is_CPP_INCLUDE(stmt))
        return stmt;

    line_after_cpp = run_cpp_on_line(line);

    stmt = repl_do_parse(line_after_cpp);

    free(line_after_cpp);

    return stmt;
}

static void evaluate_startup_file()
{
    const char *homedir;
    char *dotfile;

    if ((homedir = getenv("HOME")) == NULL) {
        homedir = getpwuid(getuid())->pw_dir;
    }

    if (!homedir)
        return;

    asprintf(&dotfile, "%s/%s", homedir, ".bic");

    if (!dotfile)
        return;


    if (access(dotfile, R_OK))
        return;

    evaluate_cscript(dotfile, false, TEXTUAL, 1, &dotfile);
}

void bic_repl()
{
    char *line;

    setup_readline();
    evaluate_startup_file();

    line = readline(BIC_PROMPT);
    while (line) {
        tree parsed_line = repl_parse_line(line);

        if (parsed_line) {
            tree result;

            add_history(line);

            result = evaluate_expr(parsed_line);

            if (result) {
                pretty_print(result);
                printf("\n");
            }
        }

        free(line);

        line = readline(BIC_PROMPT);
    }
}
