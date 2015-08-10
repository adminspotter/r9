# AX_SEARCH_LIBS_PROG(FUNCTION, SEARCH-LIBS,
#                     [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND],
#                     [OTHER-LIBRARIES], [PROGRAM])
# --------------------------------------------------------
# Search for a library defining FUNC, if it's not already available.
# If the sixth argument is provided, it should be the output of
# AC_LANG_PROGRAM.
AC_DEFUN([AX_SEARCH_LIBS_PROG],
[AS_VAR_PUSHDEF([ac_Search], [ac_cv_search_$1])dnl
AC_CACHE_CHECK([for library containing $1], [ac_Search],
[ac_func_search_save_LIBS=$LIBS
if test -z "$6"
then
  AC_LANG_CONFTEST([AC_LANG_CALL([], [$1])])
else
  AC_LANG_CONFTEST($6)
fi
for ac_lib in '' $2; do
  if test -z "$ac_lib"; then
    ac_res="none required"
  else
    ac_res=-l$ac_lib
    LIBS="-l$ac_lib $5 $ac_func_search_save_LIBS"
  fi
  AC_LINK_IFELSE([], [AS_VAR_SET([ac_Search], [$ac_res])])
  AS_VAR_SET_IF([ac_Search], [break])dnl
done
AS_VAR_SET_IF([ac_Search], , [AS_VAR_SET([ac_Search], [no])])dnl
rm conftest.$ac_ext
LIBS=$ac_func_search_save_LIBS])
ac_res=AS_VAR_GET([ac_Search])
AS_IF([test "$ac_res" != no],
  [test "$ac_res" = "none required" || LIBS="$ac_res $LIBS"
  $3],
      [$4])dnl
AS_VAR_POPDEF([ac_Search])dnl
])
