#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])
AC_INIT([pgibbs], [0.1.0], [neubig@gmail.com])
AC_CONFIG_SRCDIR(src/lib/model-base.cc)
AC_CONFIG_HEADERS([src/include/pgibbs/config.h])
AC_CONFIG_FILES([
    Makefile
    src/Makefile
    src/include/Makefile
    src/lib/Makefile
    src/bin/Makefile
    src/test/Makefile
])

AM_INIT_AUTOMAKE(pgibbs, 0.1.0)

# Uncomment to disable shared libraries (for example, for debugging)
AM_DISABLE_SHARED

# Checks for programs.
AC_PROG_LIBTOOL
AC_PROG_CXX
AC_PROG_CC

AC_LANG_CPLUSPLUS
LDFLAGS="$LDFLAGS -lpthread"

AC_CONFIG_MACRO_DIR([m4])

# Checks for libraries.
AC_SYS_LARGEFILE

# Checks for header files.
AC_CHECK_HEADERS([sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T

# Checks for library functions.
AC_CHECK_FUNCS([gettimeofday pow sqrt])

AX_CXX_COMPILE_STDCXX_0X
if test "x$ax_cv_cxx_compile_cxx0x_cxx" = "xyes" ; then
    CXXFLAGS="$CXXFLAGS -std=c++0x"
    AC_SUBST(CXXFLAGS)
fi

AC_OUTPUT
