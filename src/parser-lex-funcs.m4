define(CFILE_ONLY, `ifelse(TARGET, `cfile', `divert(0)', `divert(-1)')dnl')
define(CSCRIPT_ONLY, `ifelse(TARGET, `cscript', `divert(0)', `divert(-1)')dnl')
define(CFILE_AND_CSCRIPT_ONLY, `ifelse(TARGET, `cscript', `divert(0)', TARGET, `cfile', `divert(0)', `divert(-1)')dnl')
define(REPL_ONLY, `ifelse(TARGET, `repl', `divert(0)', `divert(-1)')dnl')
define(ALL_TARGETS, `divert(0)dnl')dnl
