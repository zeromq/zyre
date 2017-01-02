AC_DEFUN([AX_PROJECT_LOCAL_HOOK], [
    # Check for examples/chat intent
    AC_ARG_ENABLE([examples_chat],
        AS_HELP_STRING([--enable-examples-chat],
            [Compile 'chat' in examples/chat [default=no]]),
        [enable_examples_chat=$enableval],
        [enable_examples_chat=no])
    AM_CONDITIONAL([ENABLE_EXAMPLES_CHAT], [test x$enable_examples_chat != xno])
    AM_COND_IF([ENABLE_EXAMPLES_CHAT], [AC_MSG_NOTICE([ENABLE_EXAMPLES_CHAT defined])])
])
