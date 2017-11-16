#include "config.h"
#include "tree.h"
#include "typename.h"
#include "evaluate.h"
#include "gc.h"
#include "replparser.h"
#include "repllex.h"
#include "util.h"
#include "pretty-printer.h"

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

        if (text[i] == '-' && text[i + 1] == '>')
        return 1;
    }

    return 0;
}

struct completion_comp_access
{
    char *obj_expr;
    char *access_operator;
    char *member_prefix;
};

static int split_compound_access_text(const char *text,
                                      struct completion_comp_access *comp_access)
{
    size_t i, text_len = strlen(text);

    memset(comp_access, 0, sizeof(comp_access));

    for (i = text_len; i > 0; i--)
        if (text[i] == '.') {
            comp_access->obj_expr = GC_STRDUP(text);
            comp_access->obj_expr[i] = '\0';

            comp_access->access_operator = GC_STRDUP(".");

            comp_access->member_prefix = GC_STRDUP(&text[i + 1]);
            break;
        }

    if (!comp_access->obj_expr)
        return 0;

    return 1;
}

static tree get_compound_completion_object(struct completion_comp_access comp_access)
{
    char *comp_obj_expr;

    /* Append a ';' to the `obj_expr' to make it a valid C
     * expression. */
    comp_obj_expr = concat_strings(comp_access.obj_expr, ";");

    YY_BUFFER_STATE lex_buffer = repl_scan_string(comp_obj_expr);
    int parse_result = replparse();
    repl_delete_buffer(lex_buffer);

    if (parse_result)
        return NULL;

    return evaluate(repl_parse_head, "<completion>");
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

static char **bic_completion(const char *text, int start, int end)
{
    char **matches = NULL;

    if (is_compound_access(text)) {
        tree live_compound;
        struct completion_comp_access comp_access;

        if (!split_compound_access_text(text, &comp_access))
            return NULL;

        live_compound = get_compound_completion_object(comp_access);

        if (!is_T_LIVE_COMPOUND(live_compound))
            return NULL;

        matches = do_compound_completion(comp_access, text, live_compound);
    }
    else
        matches = do_identifier_completion(text);

    return matches;
}

static void setup_readline()
{
    rl_attempted_completion_function = bic_completion;
}

void bic_repl()
{
    char *line;

    setup_readline();

    line = readline(BIC_PROMPT);
    while (line) {
        int parse_result;

        YY_BUFFER_STATE buffer = repl_scan_string(line);
        parse_result = replparse();
        repl_delete_buffer(buffer);

        if (!parse_result) {
            tree result;

            add_history(line);

            result = evaluate(repl_parse_head, "<stdin>");

            if (result)
                pretty_print(result);
        }

        free(line);

        line = readline(BIC_PROMPT);
    }
}
