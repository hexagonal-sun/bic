check_PROGRAMS = puts strcat pass_parameter_printf decl_with_assign	\
                 fn_param_name_val_name_same postfix_increment		\
                 prefix_increment postfix_decrement prefix_decrement	\
                 simple_typedef simple_struct struct_typedef add sub	\
                 mul div simple_pointer set_pointer_through_function	\
                 printf_double_explicit printf_double_live_var lt gt	\
                 lteq gteq fibonacci sizeof gethostname linked-list	\
                 struct_torture array_torture union basic_enum		\
                 forward_declare double_pointer				\
                 typedef_decl_mul_ambiguity function_pointer		\
                 double_extern_fn_decl static_ints			\
                 permit_incomplete_struct_ptr logical_op_test		\
                 if_stmts eqneq union-struct casts shift loops negate	\
                 ior function_ptr_typedef array_ptr			\
                 struct_array_alignment iand factorial char-decl-and-assignment \
                 for-loop-init-decl sizeof-deref break comma modulo multi_assign \
                 xor encrypt-decrypt

expect_scripts = fileio.exp strtok.exp decl.exp chars.exp infix.exp \
                 repl-interrupt.exp


if FOUND_EXPECT
TESTS = $(check_PROGRAMS) $(expect_scripts)
else
TESTS = $(check_PROGRAMS)
endif

TEST_RUNNER = $(srcdir)/test.sh
TEST_EXTENSIONS = .exp
EXP_LOG_COMPILER = $(srcdir)/expectHarness.sh "$(srcdir)"
LOG_COMPILER = $(TEST_RUNNER)
EXTRA_DIST = $(TEST_RUNNER) $(expect_scripts) expectHarness.sh \
              utils.exp setup.exp repl-interrupt.c
