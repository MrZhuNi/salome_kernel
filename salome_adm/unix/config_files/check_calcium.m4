AC_DEFUN([CHECK_CALCIUM], [

## Guess where as cal_int type is provided by --with-cal_int option
## or auto-detection must be used

AC_ARG_WITH([cal_int],
            AC_HELP_STRING([--with-cal_int=<C type>],
	                   [Use <C type> for mapping a Fortran integer in C within CALCIUM C/F77 interfaces.]),
	    [],
	    [withval=no])

if test "x$withval" = "xno"
then
  AC_REQUIRE([CHECK_F77])
  AC_CHECK_SIZEOF_FORTRAN(integer)
  AC_CHECK_SIZEOF(long)
  AC_CHECK_SIZEOF(int)

  if test "x$ac_cv_sizeof_fortran_integer" = "x8" ; then
     AC_DEFINE(HAVE_F77INT64,[],
                       [The size of a Fortran integer, as computed by sizeof.])
     test "x$ac_cv_sizeof_long" = "x8" || AC_MSG_ERROR([Size of C type long expected to be eight bytes])
     LONG_OR_INT="long"
     CALCIUM_IDL_INT_F77="long long"
     CALCIUM_CORBA_INT_F77="CORBA::LongLong"
  elif test "x$ac_cv_sizeof_fortran_integer" = "x4" ; then
     test "x$ac_cv_sizeof_int" = "x4" || AC_MSG_ERROR([Size of C type int expected to be four bytes])
     LONG_OR_INT="int"
     CALCIUM_IDL_INT_F77="long"
     CALCIUM_CORBA_INT_F77="CORBA::Long"
  else
     AC_MSG_ERROR([Size of Fortran type integer is neither four nor eigth bytes])
  fi

else
  LONG_OR_INT="$withval" 
  CALCIUM_IDL_INT_F77="long"
  CALCIUM_CORBA_INT_F77="CORBA::Long"
  AC_MSG_NOTICE([Using C type $withval for cal_int])
fi

AC_SUBST(CALCIUM_IDL_INT_F77)
AC_SUBST(CALCIUM_CORBA_INT_F77)
AC_SUBST(LONG_OR_INT)
AC_DEFINE_UNQUOTED([CAL_INT],[$LONG_OR_INT],
	 [The C type to be used for mapping a Fortran integer in C within CALCIUM C/F77 interfaces.])
])
